#!/bin/bash

echo "generating..."

file="input.bin"

dd if=/dev/urandom bs=1M count=2000 of="$file" status=progress

echo "generation completed: $file"