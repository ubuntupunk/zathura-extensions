#!/bin/bash

echo "🔍 TTS Debug Test - Finding the Error"
echo "====================================="
echo ""
echo "Instructions:"
echo "1. Zathura will open with debug logging"
echo "2. Press Ctrl+T to trigger TTS"
echo "3. Look for DEBUG messages to see where it fails"
echo "4. Press Ctrl+C to exit when done"
echo ""

# Run with debug logging and filter for our debug messages
GIRARA_LOG_LEVEL=debug zathura test.pdf 2>&1 | grep -E "(DEBUG|TTS|error|Error)"