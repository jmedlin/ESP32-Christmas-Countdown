#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Christmas Countdown</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
        }
        .container {
            text-align: center;
            background: rgba(255, 255, 255, 0.1);
            padding: 40px;
            border-radius: 20px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            max-width: 650px;
        }
        h1 {
            font-size: 3em;
            margin: 0 0 20px 0;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        .countdown {
            font-size: 8em;
            font-weight: bold;
            margin: 30px 0;
            text-shadow: 3px 3px 6px rgba(0, 0, 0, 0.4);
        }
        .label {
            font-size: 1.5em;
            margin-top: 10px;
            opacity: 0.9;
        }
        .info {
            margin-top: 30px;
            font-size: 1.2em;
            opacity: 0.8;
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            background: rgba(0, 0, 0, 0.2);
            border-radius: 10px;
            font-size: 0.9em;
        }
        .status div {
            margin: 5px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎄 Christmas Countdown 🎄</h1>
        <div class="countdown" id="days">--</div>
        <div class="label">days until Christmas <span id="year">2025</span></div>
        <div class="info">
            <div id="currentDate">Loading...</div>
        </div>
        <div class="status">
            <div><strong>Network:</strong> <span id="ssid">--</span></div>
            <div><strong>IP:</strong> <span id="ip">--</span></div>
            <div><strong>Signal:</strong> <span id="rssi">--</span> dBm</div>
        </div>
    </div>
    <script>
        function updateCountdown() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('days').textContent = data.days;
                    document.getElementById('year').textContent = data.christmasYear;
                    document.getElementById('currentDate').textContent =
                        `${data.month}/${data.day}/${data.year} ${String(data.hour).padStart(2,'0')}:${String(data.min).padStart(2,'0')}:${String(data.sec).padStart(2,'0')}`;
                    document.getElementById('ssid').textContent = data.ssid;
                    document.getElementById('ip').textContent = data.ip;
                    document.getElementById('rssi').textContent = data.rssi;
                })
                .catch(error => console.error('Error:', error));
        }

        // Update immediately and then every 5 seconds
        updateCountdown();
        setInterval(updateCountdown, 5000);
    </script>
</body>
</html>
)rawliteral";

#endif
