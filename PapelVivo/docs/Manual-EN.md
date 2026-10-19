# Player manual — Papel Vivo 2.0.1
## 1. Installation and files
64-bit Windows 10/11, a GPU/driver compatible with libmpv/D3D11. Per-user setup, no administrator privileges. Choose a product-only folder, not a drive root or another program's directory. Python/rembg, model and video tools are bundled for offline use. The first background-removal operation may take longer.
Application source: Sources/PapelVivo-2.0.1-Fontes.zip. Library sources and recipes: Sources/ThirdParty. No scene-authoring application is installed.

## 2. Open, apply and replace
Normal startup only selects the last media; it never starts a wallpaper or assumes one is playing. A Brisa Azul sample is included. Local MP4, WebM, MKV, AVI, MOV and PNG/JPEG/BMP/WebP images are supported according to libmpv codecs. Selecting another item does not interrupt the current one; replacement follows preparation. If preparation fails, check Diagnostics. A playback-engine load failure can stop the previous media; apply it again from Gallery.
The window opens compactly and is bounded by the monitor work area. Use scrollbars or the mouse wheel when content does not fit; Shift+wheel scrolls horizontally. Tab reveals the focused control. During moving/resizing, the live preview is temporarily hidden; its viewport updates after movement ends without restarting the engine.
The central preview preserves aspect ratio and grows with the window; when not playing, it shows a real thumbnail. Test preview plays on an embedded video surface. Apply to desktop uses the existing behind-icons attachment. Alternative mode is a non-interactive overlay and can cover desktop icons. Restore original undoes it.

## 3. Heavy videos and performance
The app probes resolution, FPS and bitrate and detects CPU threads, memory and display adapter. Above 1080p, 60 FPS or 20 Mbps is heavy; systems with at most two threads or less than 6 GB use 720p, 30 FPS or 6 Mbps thresholds. The Player automatically prepares an H.264 copy without altering the original, attempting NVENC and hardware Media Foundation encoding before two-thread libx264 CPU fallback. The copy is added to Gallery.
Output is limited to 1080p/30, or 720p/30 on lower-capacity systems, preserving aspect ratio and bounding bitrate. This does not guarantee stutter-free playback or hardware encoding on every GPU. Audio is stream-copied when MP4-compatible; incompatible codecs may prevent optimization. If all attempts fail, a character dialog offers cancellation or the original. Stop cancels the helper processes.
App/system CPU and RAM are real readings. GPU information is the adapter and current decoding mode, not an invented utilization percentage. Sustained high pressure produces a brief character notice that fades in/out and lets clicks pass through. A cooldown avoids repeated interruptions. Windows reduced-animation preferences are respected.
30/60 FPS is a presentation cap, not frame interpolation. Volume zero is muted. Full-screen-app and session-lock pauses reduce load using the existing engine behavior.

## 4. Gallery and undo
Gallery stores file references, friendly names and thumbnails. To rename: select a thumbnail, click **Rename**, type the new name and click **Save name** or press **Enter**. **Esc** cancels. The field starts read-only. Incoming thumbnails must not overwrite the editing draft. Confirmed labels persist after reopening Gallery and the app. Rename only changes the displayed label, never the original filename. Remove only removes the entry; removing the playing entry stops playback and restores the desktop. It never deletes original media. Ctrl+Z undoes rename/remove; Ctrl+Y redoes within the current session, up to 100 history states. The edit box keeps normal text-editing shortcuts; click the list before undoing a gallery action.
Double-click or Apply replaces current media. Keep referenced originals in place; moving/deleting them outside the app requires choosing them again. Empty/duplicate labels are rejected. The small character briefly reacts to real actions.

## 5. Background removal
Select an image and click Remove background. If a video is selected, choose an image in the file picker. Python/rembg and U2Netp execute locally in an isolated process, without downloading models. Input limits: 128 MB, 8192 pixels per axis, 32 megapixels. Output is bounded to 4096×4096 with preserved aspect ratio, saved as a transparent PNG copy and added to Gallery. Check the result: small details may be lost. Originals are unchanged. This tool does not cut out videos.
Starting the tool shows: “Processing image... This may use significant CPU for a few seconds.” Remove background is disabled while that job runs. Processing remains in an isolated helper process, off the window thread. Actual duration depends on hardware and input and may exceed a few seconds.
Errors and timeouts produce notices and logs, never fake success. Stop/close terminates Python along with the Player. Generated files stay in the application data folder.

## 6. Close, restore and uninstall
Restore original restores the desktop and keeps the panel open. Stop and exit terminates everything. X with an active wallpaper explains: “Papel Vivo will close. Your original wallpaper will return to normal. See you next time!” Cancel keeps it running; Continue closes.
The launcher supervises Core with a KILL_ON_JOB_CLOSE Job Object; helpers belong to the process chain. No resident service. Forced termination may prevent normal Windows visual cleanup; reopen and restore if needed.
Uninstall asks **Delete saved videos and settings too?** The option is unchecked by default. Unchecked keeps Player data. Checked removes only %LOCALAPPDATA%\PapelVivo\Player, including generated copies, thumbnails, gallery, logs and preferences. Original files in other directories are never deleted. Back up generated copies you need. Silent uninstall preserves data by default.

## 7. Diagnostics and privacy
Data: %LOCALAPPDATA%\PapelVivo\Player. Logs subfolder contains PapelVivo.log and media-tools.log. Background-removal job records, including worker.log and traceback when available, are under Removed. Diagnostics offers Copy report and Open logs folder.
Logs may contain private paths; review before sharing. No automatic file upload or network telemetry. The app does not verify media rights; respect copyright.
If wallpaper is missing, check graphics drivers, full-screen pause, Preview and logs. Alternative overlay is optional. --sem-aceleracao is a diagnostic option and can increase CPU usage, not an optimization.

## 8. Version 2.0.1 interface and validation
The UI uses buffered painting, DWM-synchronized presentation when composition is available, limited invalidation and finite animations. libmpv VSync settings are unchanged. Buttons use embedded monochrome vector-derived PNG icons, without emoji or symbol-font dependencies.
This revision was tested on Linux + Wine 11.19. No affected-PC log accompanied the original move/resize crash report. This revision limits live-video viewport resizing while dragging; it does not claim a proven root cause or universal crash prevention. Physical Windows, D3D11, multi-monitor/DPI, hover and resize validation remain required. The installer is unsigned.
