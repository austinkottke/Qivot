#!/bin/bash
for f in *.sql; do
    echo "=== $f ==="
    tables=$(grep -c "CREATE TABLE" "$f" 2>/dev/null || echo 0)
    echo "Tables: $tables"
    
    # Count foreign keys
    fk=$(grep -c "FOREIGN KEY" "$f" 2>/dev/null || echo 0)
    echo "Foreign Keys: $fk"
    
    # Count unique constraints
    unique=$(grep -c "UNIQUE" "$f" 2>/dev/null || echo 0)
    echo "Unique Constraints: $unique"
    
    # Check for JSON
    json=$(grep -c "JSON" "$f" 2>/dev/null || echo 0)
    [[ $json -gt 0 ]] && echo "JSON/JSONB: YES" || echo "JSON/JSONB: NO"
    
    # Check for arrays
    arrays=$(grep -c "\\[\\]" "$f" 2>/dev/null || echo 0)
    [[ $arrays -gt 0 ]] && echo "Array Types: YES" || echo "Array Types: NO"
    
    # Check for ENUM
    enum=$(grep -c "ENUM" "$f" 2>/dev/null || echo 0)
    [[ $enum -gt 0 ]] && echo "ENUM: YES" || echo "ENUM: NO"
    
    # Check for full-text search
    fts=$(grep -c "FULLTEXT" "$f" 2>/dev/null || echo 0)
    [[ $fts -gt 0 ]] && echo "Full-Text Search: YES" || echo "Full-Text Search: NO"
    
    echo ""
done
