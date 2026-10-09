#!/usr/bin/env bash
set -euo pipefail

broker_ip="10.42.0.1"
config_file="$(mktemp)"
broker_pid=""

cleanup() {
	if [[ -n "$broker_pid" ]]; then
		kill "$broker_pid" 2>/dev/null || true
		wait "$broker_pid" 2>/dev/null || true
	fi   

	rm -f "$config_file"
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

cat > "$config_file" <<EOF
listener 1883 $broker_ip
allow_anonymous true
EOF

mosquitto -c "$config_file" &
broker_pid=$!

sleep 1

if ! kill -0 "$broker_pid" 2>/dev/null; then
	echo "Broker failed to start. Check the error above."
	exit 1
fi

echo "Broker running at $broker_ip:1883"
echo "Listening to greenhouse/01/# — press Ctrl+C to stop."

mosquitto_sub \
	-h "$broker_ip" \
	-p 1883 \
	-t 'greenhouse/01/#' \
	-v
