// Static signature verification using a caller-supplied factory public key.
// A key extracted from an unauthenticated archive is not an independent trust root.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const [meta,keyFile,checksumOverride]=process.argv.slice(2);
if(!meta||!keyFile)throw new Error('usage: node verify-swup-signature.mjs META_DIRECTORY FACTORY_PUBLIC_KEY');
const signatureFile=path.join(meta,'main.mnf_cks_S.sig');
const ini=fs.readFileSync(signatureFile,'utf8');
const signed=ini.match(/^SignedFile\s*=\s*"([^"\r\n]+)"/m)?.[1];
if(!signed||path.basename(signed)!==signed||signed.includes('\\'))throw new Error('Unsafe SignedFile');
const chunks=[...ini.matchAll(/^signature(\d+)\s*=\s*"([0-9a-fA-F]{32})"/gm)].sort((a,b)=>Number(a[1])-Number(b[1]));
if(!chunks.length||chunks.some((c,i)=>Number(c[1])!==i+1))throw new Error('Invalid signature sequence');
const signature=Buffer.from(chunks.map(c=>c[2]).join(''),'hex');
// Some distributed containers rename a versioned checksum file to main.mnf.cks.
// An explicit override verifies those bytes without modifying the signed data.
const checksumFile=checksumOverride||path.join(meta,signed);
const payload=fs.readFileSync(checksumFile);
const pem=fs.readFileSync(keyFile);
const key=crypto.createPublicKey(pem);
const verified=crypto.verify('sha256',payload,{key,padding:crypto.constants.RSA_PKCS1_PSS_PADDING,saltLength:crypto.constants.RSA_PSS_SALTLEN_AUTO},signature);
console.log(JSON.stringify({signedFile:signed,checkedFile:path.basename(checksumFile),signatureBytes:signature.length,algorithm:'RSA-PSS SHA-256',verifiedAgainstSuppliedKey:verified,publicKeySha256:crypto.createHash('sha256').update(pem).digest('hex'),independentRootOfTrustVerified:false,vehicleAcceptanceVerified:false},null,2));
if(!verified)process.exitCode=1;
