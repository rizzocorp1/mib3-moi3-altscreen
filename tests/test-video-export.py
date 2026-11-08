"""Native private exporter: exact copies, malformed inputs and evidence retention."""
from pathlib import Path
import os,struct,subprocess,sys,tempfile
guard=Path(sys.argv[1]).resolve()
def record(pts=90000,body=b'\0\0\0\1\x65synthetic'):
    return struct.pack('<IQ',len(body),pts)+body
valid=b'MOI3VC1\n'+record()+record(93000)
scenarios=['normal','matching-output','different-output','same-as-source','source-link','marker-link','output-link',
           'bad-magic','bad-token','short-record','bad-length','bad-annexb','backward-pts','long-duration','too-many','oversized']
for scenario in scenarios:
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp);source=root/'video.bin';ready=root/'video.ready';destination=root/'export.bin'
        blob=valid;token=b'MOI3VC1\n'
        if scenario=='bad-magic':blob=b'UNKNOWN!'+record()
        if scenario=='bad-token':token=b'INVALID\n'
        if scenario=='short-record':blob=valid+b'123'
        if scenario=='bad-length':blob=b'MOI3VC1\n'+struct.pack('<IQ',2000000,90000)+b'short'
        if scenario=='bad-annexb':blob=b'MOI3VC1\n'+record(body=b'not-a-video-prefix')
        if scenario=='backward-pts':blob=b'MOI3VC1\n'+record()+record(89999)
        if scenario=='long-duration':blob=b'MOI3VC1\n'+record()+record(360001)
        if scenario=='too-many':blob=b'MOI3VC1\n'+b''.join(record(90000+i) for i in range(91))
        if scenario=='oversized':blob=valid+b'x'*(2*1024*1024)
        source.write_bytes(blob);source.chmod(0o600);ready.write_bytes(token);ready.chmod(0o600)
        if scenario in ('source-link','marker-link'):
            path=source if scenario=='source-link' else ready;original=root/'private-original'
            path.rename(original);path.symlink_to(original)
        if scenario=='same-as-source':destination=source
        if scenario=='matching-output':destination.write_bytes(valid)
        if scenario=='different-output':destination.write_bytes(b'preserve existing data')
        if scenario=='output-link':
            original=root/'unrelated';original.write_bytes(b'preserve unrelated');destination.symlink_to(original)
        # Absolute native binary needs no shell utilities or their optional flags.
        env=dict(os.environ,PATH=str(root/'no-utilities'))
        result=subprocess.run([guard,'video-export',source,ready,destination],env=env,capture_output=True)
        if scenario in ('normal','matching-output'):
            assert result.returncode==0,(scenario,result.stderr)
            assert destination.read_bytes()==valid and not source.exists() and not ready.exists()
            if scenario=='normal':assert destination.stat().st_mode&0o777==0o600
        else:
            assert result.returncode!=0,scenario
            assert source.read_bytes()==blob and ready.read_bytes()==token
            if scenario=='different-output':assert destination.read_bytes()==b'preserve existing data'
            if scenario=='output-link':assert destination.read_bytes()==b'preserve unrelated'
print('PASS: native private video export with no shell utilities; exact readback, completion/record/size/time bounds, existing-output and symlink refusal; failed validation preserves original evidence')
