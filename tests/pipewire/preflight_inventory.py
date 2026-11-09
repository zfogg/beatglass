"""Pure validation rules for the native PipeWire sink inventory."""


def validate_sink_inventory(inventory):
    """Return a human-readable failure, or None for an addressable sink set."""
    if not isinstance(inventory, list):
        return "valid pw-dump JSON list"
    sinks = []
    for item in inventory:
        if not isinstance(item, dict):
            continue
        props = item.get("info", {}).get("props", {})
        if props.get("media.class") == "Audio/Sink":
            sinks.append((item, props))
    if not sinks:
        return "PipeWire Audio/Sink node"
    if any(not isinstance(item.get("id"), int) or item["id"] < 0 for item, _ in sinks):
        return "numeric PipeWire output sink object id"
    serials = [str(props.get("object.serial", "")).strip() for _, props in sinks]
    if any(not serial for serial in serials):
        return "non-empty PipeWire output sink serial"
    if len(serials) != len(set(serials)):
        return "unique PipeWire output sink serials"
    names = [str(props.get("node.name", "")).strip() for _, props in sinks]
    if any(not name for name in names):
        return "non-empty PipeWire output sink node name"
    if len(names) != len(set(names)):
        return "unique PipeWire output sink node names"
    return None
