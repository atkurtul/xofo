#!/usr/bin/python3

import tkinter
from tkinter import filedialog
tkinter.Tk().withdraw()
file_path = filedialog.askopenfilename()

print(file_path)
