import sys, numpy as np
sys.path.insert(0,'/home/jyrki/pajupilli/tools')
from scipy.io import wavfile
for n in sys.argv[1:]:
    fs,x=wavfile.read('/home/jyrki/pajupilli/research/gm_compare/work/brushaim/%s.wav'%n)
    x=x.astype(float)
    if x.dtype.kind=='i' or abs(x).max()>4: x=x/32768
    print(n,x.shape,'rms dB L %.2f R %.2f'%tuple(20*np.log10(np.sqrt(np.mean(x[fs:8*fs]**2,axis=0))+1e-12)))
    w=fs//8
    row=[]
    for i in range(0,8*fs-w,w):
        l=x[i:i+w,0]; r=x[i:i+w,1]
        m=l+r; s=l-r
        row.append('%+.1f/%.2f'%(20*np.log10(np.sqrt(np.mean(l*l)/np.mean(r*r))), np.sqrt(np.mean(s*s)/np.mean(m*m))))
    print(' '.join(row[8:40]))
