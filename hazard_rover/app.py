from flask import Flask, render_template, Response, jsonify
import serial
import threading
import time
import sqlite3
from datetime import datetime
from picamera2 import Picamera2
import io

app = Flask(__name__)

# Camera setup
picam2 = Picamera2()
picam2.configure(picam2.create_video_configuration(main={"size": (640, 480)}))
picam2.start()

def generate_frames():
    while True:
        stream = io.BytesIO()
        picam2.capture_file(stream, format='jpeg')
        stream.seek(0)
        frame = stream.read()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
        time.sleep(0.1)

# Database setup
def init_db():
    conn = sqlite3.connect('rover_data.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS sensor_readings
                (id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME,
                temperature REAL,
                humidity REAL,
                gas INTEGER,
                mq2 INTEGER)''')
    conn.commit()
    conn.close()

def save_to_db(temperature, humidity, gas, mq2):
    try:
        conn = sqlite3.connect('rover_data.db')
        c = conn.cursor()
        c.execute('''INSERT INTO sensor_readings 
                    (timestamp, temperature, humidity, gas, mq2)
                    VALUES (?, ?, ?, ?, ?)''',
                    (datetime.now(), temperature, humidity, gas, mq2))
        conn.commit()
        conn.close()
    except Exception as e:
        print(f"Database error: {e}")

def get_history():
    try:
        conn = sqlite3.connect('rover_data.db')
        c = conn.cursor()
        c.execute('''SELECT timestamp, temperature, humidity, gas, mq2 
                    FROM sensor_readings 
                    ORDER BY timestamp DESC 
                    LIMIT 50''')
        rows = c.fetchall()
        conn.close()
        return rows
    except Exception as e:
        print(f"Database error: {e}")
        return []

# Serial connection
ser = None
sensor_data = {
    "temperature": "--",
    "humidity": "--",
    "gas": "--",
    "mq2": "--"
}

def read_serial():
    global ser, sensor_data
    while True:
        try:
            ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=2)
            ser.flushInput()
            while True:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8').strip()
                    if line:
                        parse_data(line)
        except Exception as e:
            print(f"Serial error: {e}")
            time.sleep(2)

def parse_data(line):
    global sensor_data
    try:
        parts = line.split(',')
        for part in parts:
            key, value = part.split(':')
            if key == 'TEMP':
                sensor_data['temperature'] = value
            elif key == 'HUM':
                sensor_data['humidity'] = value
            elif key == 'GAS':
                sensor_data['gas'] = value
            elif key == 'MQ2':
                sensor_data['mq2'] = value

        # Only save to database when we receive a complete reading
        # A complete reading contains all 4 values
        if all(key in line for key in ['TEMP', 'HUM', 'GAS', 'MQ2']):
            save_to_db(
                sensor_data['temperature'],
                sensor_data['humidity'],
                sensor_data['gas'],
                sensor_data['mq2']
            )
    except:
        pass

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

@app.route('/')
def index():
    history = get_history()
    return render_template('dashboard.html', history=history)

@app.route('/data')
def data():
    return jsonify(sensor_data)

@app.route('/history')
def history():
    rows = get_history()
    history_list = []
    for row in rows:
        history_list.append({
            'timestamp': row[0],
            'temperature': row[1],
            'humidity': row[2],
            'gas': row[3],
            'mq2': row[4]
        })
    return jsonify(history_list)

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)
