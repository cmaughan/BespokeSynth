import sampleplayer

s = sampleplayer.get("sampleplayer")

length = 44100*2
data = [0]*length
for i in range(length):
   data[i] = math.sin(i/44100 * 220 * 2 * math.pi)*.2
   
s.fill(data) 