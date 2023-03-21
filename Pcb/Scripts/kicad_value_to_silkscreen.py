import pcbnew


b = pcbnew.GetBoard()
for f in b.GetFootprints():
    r = f.Reference()
    l = r.GetLayer()
    v = f.Value()
    print("ref:" + str(f.GetReference()) + " value:" + str(f.GetValue()) + " curr layer:" + str(v.GetLayer()) + "set layer:" + str(l))
    v.SetLayer(l)
