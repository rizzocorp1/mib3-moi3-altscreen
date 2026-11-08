"""Execute the real embedded read-only discovery client against HTTP fixtures."""
from pathlib import Path
from unittest.mock import patch
import contextlib,io,json,sys
text=Path(sys.argv[1]).read_text(encoding='utf-8')
source=text.split("<<'PYDISPLAY'\n",1)[1].split('\nPYDISPLAY',1)[0]
for scenario in ('valid','missing','duplicate','external-uri','bad-port','resource-error'):
    calls=[]
    entry={'name':'displaymanagement','uri':'/displaymanagement/','port':44000}
    entries=[entry]
    if scenario=='missing':entries=[]
    if scenario=='duplicate':entries=[entry,dict(entry)]
    if scenario=='external-uri':entry['uri']='http://example.invalid/'
    if scenario=='bad-port':entry['port']=65536
    class Response:
        def __init__(self,value):self.value=value
        def read(self,n):assert n==262144;return json.dumps(self.value).encode()
        def close(self):pass
    def urlopen(url,timeout):
        assert timeout==1 and isinstance(url,str);calls.append(url)
        if url=='http://127.0.0.1/':return Response({'data':entries})
        assert url.startswith('http://127.0.0.1:44000/displaymanagement/')
        if scenario=='resource-error':raise OSError('fixture read-only denial')
        return Response({'data':[]})
    output=io.StringIO()
    with patch('urllib.request.urlopen',urlopen),contextlib.redirect_stdout(output):exec(compile(source,'embedded-display-snapshot','exec'),{})
    if scenario in ('valid','resource-error'):
        assert len(calls)==5 and all(':44000/' in url for url in calls[1:])
        assert 'DISCOVERED LOCAL DISPLAY PORT 44000' in output.getvalue()
    else:assert calls==['http://127.0.0.1/']
print('PASS: actual embedded client follows one advertised local service port; rejects absent/ambiguous/external/invalid endpoints; bounded read-only requests and resource failures handled')
