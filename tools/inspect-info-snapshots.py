"""Read local capability snapshots; print only protocol features and geometry.
Device IDs, names, addresses, UUIDs and key material are deliberately omitted.
"""
import json,plistlib,sys
from pathlib import Path

geometry={'type','widthPixels','heightPixels','widthPhysical','heightPhysical','maxFPS',
          'features','viewAreas','initialViewArea','safeArea','x','y','width','height','id'}
def clean_geometry(value):
    if isinstance(value,dict):return {k:clean_geometry(v) for k,v in value.items() if k in geometry}
    if isinstance(value,list):return [clean_geometry(v) for v in value]
    if isinstance(value,(int,float,bool)) or value is None:return value
    return {'valueType':type(value).__name__}
for argument in sys.argv[1:]:
    path=Path(argument)
    if path.stat().st_size>5*1024*1024:raise ValueError('Snapshot exceeds expected size')
    with path.open('rb') as file:data=plistlib.load(file)
    if not isinstance(data,dict):raise ValueError('Expected root dictionary')
    features=data.get('enabledFeatures')
    displays=data.get('displays')
    result={'file':path.name,'rootKeys':sorted(data),'enabledFeaturesType':type(features).__name__,
            'enabledFeatures':features if isinstance(features,list) and all(isinstance(x,str) for x in features) else None,
            'features':data.get('features') if isinstance(data.get('features'),int) else None,
            'displaysType':type(displays).__name__,'displays':clean_geometry(displays)}
    print(json.dumps(result,indent=2))
