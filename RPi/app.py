from flask import Flask, jsonify
import click
import sqlite3
import serial
from serial.serialutil import SerialException
import threading
import time
import sys

app = Flask(__name__)
serial_port = None

def init_db():
    """Initialize SQLite database with nodes table"""
    conn = sqlite3.connect('nodes.db')
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS nodes
        (id INTEGER PRIMARY KEY AUTOINCREMENT,
         mac_address TEXT UNIQUE NOT NULL,
         name TEXT NOT NULL)
    ''')
    conn.commit()
    conn.close()

def get_serial_connection():
    """Get or create serial connection to ESP8266"""
    global serial_port
    if serial_port is None:
        try:
            serial_port = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
            time.sleep(2)  # Wait for ESP8266 to initialize
        except SerialException as e:
            print(f"Error connecting to serial port: {e}")
            return None
    return serial_port

@app.route('/node/<int:node_id>/<int:state>')
def control_node(node_id, state):
    """API endpoint to control node state by node ID"""
    if state not in [0, 1]:
        return jsonify({"error": "State must be 0 or 1"}), 400
    
    # Get node MAC address from database using node_id
    conn = sqlite3.connect('nodes.db')
    c = conn.cursor()
    c.execute('SELECT mac_address, name FROM nodes WHERE id = ?', (node_id,))
    result = c.fetchone()
    conn.close()
    
    if not result:
        return jsonify({"error": f"No node found with ID {node_id}"}), 404
    
    mac_address, name = result
    serial_conn = get_serial_connection()
    
    if serial_conn is None:
        return jsonify({"error": "Serial connection failed"}), 500
    
    try:
        message = f"{mac_address},{state}\n"
        serial_conn.write(message.encode())
        return jsonify({
            "status": "success",
            "message": f"Sent {message.strip()} to device",
            "node_id": node_id,
            "node_name": name,
            "mac_address": mac_address,
            "state": state
        })
    except Exception as e:
        return jsonify({"error": f"Failed to send message: {str(e)}"}), 500

def cleanup():
    """Cleanup function to close serial connection"""
    global serial_port
    if serial_port is not None:
        serial_port.close()

# Create a Click group
@click.group()
def cli():
    """Node management CLI"""
    pass

@cli.command()
@click.argument('mac_address')
@click.argument('name')
def add_node(mac_address, name):
    """Add a new node to the database"""
    init_db()  # Ensure database exists
    conn = sqlite3.connect('nodes.db')
    c = conn.cursor()
    try:
        c.execute('INSERT INTO nodes (mac_address, name) VALUES (?, ?)',
                 (mac_address, name))
        conn.commit()
        click.echo(f"Added node: {name} ({mac_address})")
    except sqlite3.IntegrityError:
        click.echo(f"Error: MAC address {mac_address} already exists")
    finally:
        conn.close()

@cli.command()
def list_nodes():
    """List all nodes in the database"""
    init_db()  # Ensure database exists
    conn = sqlite3.connect('nodes.db')
    c = conn.cursor()
    c.execute('SELECT id, mac_address, name FROM nodes')
    nodes = c.fetchall()
    conn.close()
    
    if not nodes:
        click.echo("No nodes found")
        return
    
    click.echo("Registered Nodes:")
    for node_id, mac_address, name in nodes:
        click.echo(f"ID: {node_id} - {name} ({mac_address})")


@cli.command()
def run_server():
    """Run the Flask server"""
    init_db()
    app.run(debug=True)

# Register cleanup function
import atexit
atexit.register(cleanup)

if __name__ == '__main__':
    cli()
