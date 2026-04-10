from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# 🔥 UPDATED DATA STRUCTURE (MATCHES YOUR ESP32)
latest_data = {
    "temp": 0,
    "humidity": 0,
    "aq": 0,
    "light": 0,
    "pressure": 1013,
    "rain": 0,
    "motion": 0,
    "wind": 0,          # ✅ NEW
    "vibration": 0,     # ✅ NEW
    "window_open": 0,
    "servo_angle": 0,
    "relay": 0,
    "wifi": -55,
    "heap": 200
}

@app.route('/')
def home():
    return "✅ Smart Window Server is running. Use /api/data"

# 🔴 RECEIVE DATA FROM ESP32
@app.route('/api/data', methods=['POST'])
@app.route('/api/data/', methods=['POST'])
def receive_data():
    global latest_data

    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "No data received"}), 400

    # 🔥 SAFE UPDATE (only update existing keys)
    for key in latest_data:
        if key in data:
            latest_data[key] = data[key]

    print("📡 Received:", latest_data)
    return jsonify({"status": "ok"}), 200

# 🟢 SEND DATA TO HTML DASHBOARD
@app.route('/api/data', methods=['GET'])
@app.route('/api/data/', methods=['GET'])
def send_data():
    return jsonify(latest_data), 200

# 🚀 RUN SERVER
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
