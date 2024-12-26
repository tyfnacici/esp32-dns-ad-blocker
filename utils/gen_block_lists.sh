#!/bin/bash
# Script for downloading and converting the ad server lists to smaller files

HOSTS_URL="https://raw.githubusercontent.com/turkishlitecoin/PiHole/refs/heads/master/Lists/TickTock_URLs"
HOSTS_TEMP="/tmp/host_list"
OUTPUT_DIR="../data"

# Create clean output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Download and clean hosts file
wget "$HOSTS_URL" -O - | grep -v '#' | grep "0.0.0.0" | sed 's/0\.0\.0\.0 //g' > "$HOSTS_TEMP"

# Process each domain
echo "Processing domains..."
while IFS= read -r domain; do
    # Clean the domain name
    domain=$(echo "$domain" | tr -d '\r\n')
    length=${#domain}
    echo -n ",$domain," >> "$OUTPUT_DIR/hosts_$length"
done < "$HOSTS_TEMP"

# Add EOF marker to each file
for file in "$OUTPUT_DIR"/hosts_*; do
    echo -n "@@@" >> "$file"
    echo "Created: $file"
done

rm "$HOSTS_TEMP"
echo "Done! Files created in $OUTPUT_DIR"