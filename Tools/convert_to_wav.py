#!/bin/env python3

from PCM2Wav import *
import sys
output = PCM2Wav(PCM2Wav.saleae.I2S, sys.argv[1], sys.argv[1].replace(".txt", ".wav"))
