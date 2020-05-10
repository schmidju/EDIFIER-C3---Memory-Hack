
def decode(line):
	global g_data
	global g_address
	global g_write

	line = line.rstrip()
	splt = line.split(b': ')
	annon = splt[0]

	# switch
	if (annon == b'Start'):
		g_data = b''
		g_address = b''
	
	elif(annon == b'Write'):
		g_write = True		
	elif(annon == b'Read'):
		g_write = False

	elif(annon[:7] == b'Address'):
		g_address = splt[1]

	elif(annon[:4] == b'Data'):
		g_data += splt[1]
	
	elif (annon == b'Stop'):
		parse(g_address, g_data, g_write)


def parse(address, data, write):
	if (data != b''):
		if (write):	
			print(data, b' -> ', address)		

			cmd = valueToCmd(int(data[2:4], base=16))
			if (cmd == 'ON'):
				volume = valueToVolume(int(data[:2], base=16))
				src = valueToSource(int(data[11:12], base=16))
				bass = valueToBass(int(data[2:4], base=16))
				treb = valueToTreb(int(data[9:10], base=16))
				print('ON - Volume: ' + str(volume) + ' Input: ' + src + ' Bass: ' + str(bass) + ' Treb: ' + str(treb))
			elif (cmd == 'RESUME'):
				print('RESUME')
			else:
				print ('Mode Err')

		else :	
			print(data, b' <- ', address)

def valueToVolume(value):
	value = 63 - value

	if (value <= 26):
		return int(value / 2)
	elif (value <= 53):
		return value - 13
	else:
		return 40 + int((value - 53) * 2)

def valueToSource(value):
	if ((value & 1) == 1):
		return 'PC'
	else:
		return 'AUX'

def valueToBass(value):
	bass = 5 - (value & 0x1F)

	if(bass >= 0):
		return int(bass * 2)
	else:
		return int(bass / 2)

def valueToTreb(value):
	if (value <= 7):
		return value - 7
	else:
		return 15 - value

def valueToCmd(value):
	if ((value & 0xC0) == 0xC0):
		return 'ON'
	elif ((value & 0xC0) == 0x80):
		return 'RESUME'
	else:
		return 'ERR'