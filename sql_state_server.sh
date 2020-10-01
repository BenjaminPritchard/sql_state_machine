#!/bin/bash
echo 'listening on port 4242...'
socat tcp-listen:4242,reuseaddr exec:./sql_state_engine