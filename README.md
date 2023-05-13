# ExportTools
Tools to hack certain game.

## Files:
1.CrystalTile2 is a hex editor, I use it mainly for compressing and decompressing files, and modify font files.

PS1: Some .exe files would call it and the path was hardcoded in .cpp files as "E:\\CrystalTile2\\", so you need extract it to the right path, or change the .cpp files.

PS2: Try launch CrystalTile2.exe first to make sure it can run without error. If it says lack of some "DLL" files then you need to download and put them under CrystalTile2 folder.

2.NDSTool is used for dump/pack rom file, extract the rar and put rom file with .bat and .exe files then run "_dump.bat" to dump files. After changed some files then run "_pack.bat" to repack rom.

3.EXEs are the execute files of .cpp files in the "Codes" folder.

4.Pics show all those texts in game can be exported and need to trans.


## Steps:
1.Prepare Tools:

Make sure the path of CrystalTile2 and try run it.

2.Dump ROM:

Use NDSTool to unpack rom file. After unpack, there's a "data" folder with many .ccb files.

3.Export TXT:

Find the ccb files shown on Pics, use the .exe to extract them and export .txt files. You can find the Shift-JIS.tbl file under CrystalTile2 folder.

4.Translate:

Trans all .txt files, the first line is source language, and the second line is what you need to change. Do not modify the format.

5.Create TBL:

 Grab all texts, use "GetChar.exe" to filter repeated chars, then use "CreateTBL.exe" to create a new tbl file.

PS: Step 5-6 are not neccessary if the default tbl contains all your language chars. For example the Shift-JIS.tbl contains all a-z/A-Z english chars then the US Ver do not need remake .tbl file.

6.Font:

Use "dump_de.exe" to decompress f10.ccb/f12.ccb and get .nftr font files. Use the new .tbl file and the CrystalTile2(or any other font tools you know) to edit font files, then use "pack_co.exe" to compress font files.

7.Import TXT:

After translate, change old JIS.tbl with new .tbl, then import .txt files, and Repack .ccb files.

8.Pack ROM:

Replace all changed .ccb files, then use NDSTool to pack ROM.

9.Finish:
Test play with the new rom and enjoy!

## Example:
The pic "2_gameparam.png" means you need find gameparam.ccb first, then put it with "dump_de.exe", run dump_de.exe then you will get a new "gameparam" folder, enter the new folder, then put the .tbl file and "DeOut.exe" into the folder, run "DeOut.exe", you will get some new files, the .txt files are what you need change.

After trans and the .tbl file changed, then run "DeIN.exe", the .ccbgp will be changed, then back to parent folder, run "pack_co.exe", you will get a new gameparam.ccb.

PS: dump.exe/dump_de.exe/dump_main.exe are not same, they fit different scenes, such as FindText.exe/FindText2.exe and UpdText.exe/UpdText2.exe.
