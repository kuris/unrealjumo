"""Create deterministic, original mono wind/surf WAV loops without external assets."""
from array import array
import math
from pathlib import Path
import random
import wave
root=Path(__file__).resolve().parents[2]
folder=root/'Source/Audio'
folder.mkdir(parents=True,exist_ok=True)
rate=44100
for name,seed in [('Wind',9801),('Surf',9802)]:
 rng=random.Random(seed)
 samples=[]
 low=soft=fast=0.
 for i in range(rate*32):
  t=i/rate
  noise=rng.uniform(-1,1)
  low+=0.003*(noise-low)
  soft+=0.04*(noise-soft)
  fast+=0.22*(noise-fast)
  if name=='Wind':
   gust=0.55+0.22*math.sin(math.tau*t/32)+0.12*math.sin(math.tau*t/16+0.7)
   value=(0.75*low+0.3*soft+0.025*fast)*gust
  else:
   swell=(0.5+0.5*math.sin(math.tau*t/8-0.8))**2.6
   value=0.3*low+(0.17+0.83*swell)*(0.5*soft+0.18*fast)
  samples.append(value)
 cross=int(rate*1.5)
 for i in range(cross):
  weight=i/cross
  samples[i]=samples[-cross+i]*(1-weight)+samples[i]*weight
 samples=samples[:-cross]
 gain=(0.58 if name=='Wind' else 0.72)/max(abs(v) for v in samples)
 data=array('h',(max(-32767,min(32767,round(v*gain*32767))) for v in samples))
 path=folder/('Island_'+name+'.wav')
 with wave.open(str(path),'wb') as output:
  output.setnchannels(1);output.setsampwidth(2);output.setframerate(rate);output.writeframes(data.tobytes())
 print(path.name,round(len(data)/rate,2),'seconds')
