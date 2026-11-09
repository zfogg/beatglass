"""Shared canonical operations for PipeWire qualification manifests."""
import hashlib
import json
import re

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
    if any("media.class" in item["info"]["props"] and not isinstance(item["info"]["props"]["media.class"], str) for item in inventory):
        return "inventory media.class values must be strings"
    if any(not isinstance(item.get("id"), int) or item["id"] < 0 for item in inventory):
        return "inventory entries must have non-negative ids"
    ids = [item["id"] for item in inventory]
    if len(ids) != len(set(ids)):
        return "inventory object ids must be unique"
    if audio_sink_count(inventory) < 1:
        return "inventory must include an Audio/Sink"
    sinks = [item["info"]["props"] for item in inventory if item["info"]["props"].get("media.class") == "Audio/Sink"]
    if any(not isinstance(p.get("object.serial"), str) or not re.fullmatch(r"[0-9]+", p["object.serial"]) or not isinstance(p.get("node.name"), str) or not p["node.name"].strip() for p in sinks):
        return "Audio/Sink entries must retain serial and node name strings"
    return None
