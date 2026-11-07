# A PE import allowlist is build evidence, not proof of mapped code at runtime.
import re
import subprocess
import sys

ALLOWED = {"kernel32.dll", "msvcrt.dll"}

def validate(output):
    imports = {name.lower() for name in re.findall(r"DLL Name:\s*(\S+)", output)}
    if not imports or not imports <= ALLOWED:
        raise ValueError("unexpected or missing imports: " + ", ".join(sorted(imports)))
    return imports

def main():
    output = subprocess.check_output([sys.argv[1], "-p", sys.argv[2]], text=True)
    print("Observed import allowlist:", ", ".join(sorted(validate(output))))

if __name__ == "__main__":
    main()
