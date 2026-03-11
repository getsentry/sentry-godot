def read_property(prop_name, file_path):
    """Read property from .properties file"""
    with open(file_path, 'r') as file:
        for line in file:
            if line.startswith(prop_name + "="):
                return line.split('=', 1)[1].strip().strip('"')
    raise KeyError(f"Property '{prop_name}' not found in {file_path}")
