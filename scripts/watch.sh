#! /bin/sh
fswatch -o src/ | xargs -I {} ./scripts/copy.sh {}
