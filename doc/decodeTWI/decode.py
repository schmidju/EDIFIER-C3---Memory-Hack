import subprocess, sys
from importlib import reload
import edifier

command = 'C:/Program Files (x86)/sigrok/sigrok-cli/sigrok-cli.exe -d fx2lafw -c samplerate=1000000 -C D0,D1 --continuous -P i2c:scl=D1:sda=D0:address_format=unshifted '
packages = '-A i2c=start:stop:address-read:address-write:data-read:data-write'

proc = subprocess.Popen(command + packages,stdout=subprocess.PIPE)

sys.stdout.flush()
for line in iter(proc.stdout.readline, b''):
	sys.stdout.flush()
	try:
		reload(edifier)
		edifier.decode(line[7:])
	except Exception as e:
		print('error', e)
