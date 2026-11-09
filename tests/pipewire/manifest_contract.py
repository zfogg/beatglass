"""Shared canonical operations for PipeWire qualification manifests."""
import hashlib
import json

def inventory_digest(inventory):
    return hashlib.sha256(json.dumps(inventory, sort_keys=True, separators=(",", ":")).encode()).hexdigest()

def audio_sink_count(inventory):
    return sum(1 for item in inventory if isinstance(item, dict)
               and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink")
