"""Offline background-removal helper. No network downloads; originals are read-only."""
import argparse,os,json,traceback,faulthandler,hashlib
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--job',required=True);parser.add_argument('--result',required=True);args=parser.parse_args();root=Path(args.result).parent;root.mkdir(parents=True,exist_ok=True)
fault=open(root/'python-fault.log','a',buffering=1);faulthandler.enable(file=fault,all_threads=True)
try:
 job=json.loads(Path(args.job).read_text());source=Path(job['input']);base=Path(__file__).resolve().parent.parent;model=base/'modelos/u2netp.onnx'
 if not source.is_file() or source.stat().st_size>128*1024*1024:raise ValueError('Invalid input or file exceeds 128 MB')
 if hashlib.sha256(model.read_bytes()).hexdigest()!='309c8469258dda742793dce0ebea8e6dd393174f89934733ecc8b14c76f4ddd8':raise ValueError('Background-removal model missing or damaged. Reinstall the full package.')
 os.environ['U2NET_HOME']=str(model.parent);os.environ['NUMBA_CACHE_DIR']=str(root/'cache');os.environ['OMP_NUM_THREADS']='2'
 from PIL import Image,ImageOps
 Image.MAX_IMAGE_PIXELS=32_000_000
 with Image.open(source) as im:
  if im.width*im.height>32_000_000 or max(im.size)>8192:raise ValueError('Image exceeds 8192 pixels per axis or 32 MP')
  image=ImageOps.exif_transpose(im).convert('RGBA');image.thumbnail((4096,4096),Image.Resampling.LANCZOS)
 from rembg import new_session,remove
 session=new_session('u2netp',providers=['CPUExecutionProvider']);result=remove(image,session=session);output=root/'transparent.png';result.save(output,'PNG')
 data={'status':'ok','output':str(output),'width':result.width,'height':result.height}
except Exception as e:
 traceback.print_exc();data={'status':'error','error':type(e).__name__+': '+str(e),'traceback':traceback.format_exc()}
Path(args.result).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
