"""Gera a lista explícita de instalação/remoção a partir do staging revisado.
Uso: python installer/gerar_payload.py caminho/do/stage
Não use uma pasta com credenciais, builds intermediárias ou arquivos pessoais.
"""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
files=sorted(p for p in root.rglob('*') if p.is_file())
for p in files:
 # Certifi contains public CA certificates, not credentials or signing keys.
 if p.relative_to(root).as_posix()=='python/Lib/site-packages/certifi/cacert.pem':
  data=p.read_bytes()
  if b'-----BEGIN CERTIFICATE-----' in data and b'PRIVATE KEY' not in data:continue
 if p.suffix.lower() in {'.jks','.keystore','.pfx','.p12','.pem','.key'} or p.name=='.env':
  raise SystemExit('Arquivo sensível não permitido no staging: '+str(p.relative_to(root)))
lines=['; Lista explícita, gerada após revisão do staging.','!macro ExtractPayload']
for p in files:
 rel=str(p.relative_to(root)).replace('/','\\'); parent=str(p.relative_to(root).parent).replace('/','\\'); parent='' if parent=='.' else '\\'+parent
 lines+=[' SetOutPath "$NewDir'+parent+'"',' ClearErrors',' File "/oname='+p.name+'" "${PAYLOAD_ROOT}\\'+rel+'"',' IfErrors install_fail',' IfFileExists "$NewDir\\'+rel+'" 0 install_fail',' Push "$(ExtractedText) '+rel+'"',' Call Log']
lines+=['!macroend','!macro DeleteOwned BASE']
for p in files:
 rel=str(p.relative_to(root)).replace('/','\\')
 lines+=[' IfFileExists "${BASE}\\'+rel+'" 0 +5',' ClearErrors',' Delete "${BASE}\\'+rel+'"',' IfErrors 0 +2',' StrCpy $RemovalError 1']
for d in sorted((p for p in root.rglob('*') if p.is_dir()),key=lambda p:-len(str(p))):
 lines+=[' RMDir "${BASE}\\'+str(d.relative_to(root)).replace('/','\\')+'"']
lines+=['!macroend']
Path(__file__).with_name('payload.nsh').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('Arquivos explícitos:',len(files))
