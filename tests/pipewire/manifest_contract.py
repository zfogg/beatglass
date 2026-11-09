"""Shared canonical operations for PipeWire qualification manifests."""
import hashlib
import json

def inventory_digest(inventory):
    return hashlib.sha256(json.dumps(inventory, sort_keys=True, separators=(",", ":")).encode()).hexdigest()

def audio_sink_count(inventory):
    return sum(1 for item in inventory if isinstance(item, dict)
               and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink")

def validate_inventory_shape(inventory):
    if not isinstance(inventory, list) or not inventory:
        return "inventory must be a non-empty list"
    if any(not isinstance(item, dict) for item in inventory):
        return "inventory entries must be objects"
    if any(not isinstance(item.get("info"), dict) or not isinstance(item["info"].get("props"), dict) for item in inventory):
        return "inventory entries must retain info.props objects"
    if any(not isinstance(item.get("id"), int) or item["id"] < 0 for item in inventory):
        return "inventory entries must have non-negative ids"
    if audio_sink_count(inventory) < 1:
        return "inventory must include an Audio/Sink"
    return None
