"""Upload the prepared fairy through Mixamo's visible file input in Chrome.

Uses browser DOM only; does not access credentials, storage or private APIs.
"""
import base64
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
source = root / 'Saved/Swimming/Source/Fairy_CurrentGame_Mixamo.fbx'

def execute(javascript):
    script = '''tell application "Google Chrome"
 repeat with w in windows
  repeat with t in tabs of w
   if URL of t contains "mixamo.com" then
    return execute t javascript %s
   end if
  end repeat
 end repeat
 error "No Mixamo tab found"
end tell''' % json.dumps(javascript)
    result = subprocess.run(['osascript','-'],input=script,text=True,capture_output=True,check=True)
    return result.stdout.strip()

print(execute("window.fairyUploadParts=[]; 'upload initialized'"),flush=True)
data = source.read_bytes()
for start in range(0,len(data),180000):
    chunk = base64.b64encode(data[start:start+180000]).decode()
    execute("window.fairyUploadParts.push(Uint8Array.from(atob("+json.dumps(chunk)+"), c=>c.charCodeAt(0))); 'chunk accepted'")
javascript = """(() => {
const input=document.querySelector('input[type=file]');
if(!input) throw new Error('Open Upload Character dialog first');
const file=new File(window.fairyUploadParts,['Fairy_CurrentGame_Mixamo.fbx'],{type:'application/octet-stream'});
const transfer=new DataTransfer(); transfer.items.add(file); input.files=transfer.files;
input.dispatchEvent(new Event('change',{bubbles:true}));
delete window.fairyUploadParts; return 'Current fairy FBX selected: '+file.size+' bytes';
})()"""
print(execute(javascript),flush=True)
