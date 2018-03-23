# PSVita-ELF-rebuilder by CelesteBlue

Credits: zecoxao for the method to rebuild ELF from segments

## Usage :

1) In os0-, ud0- and vs0-, place the REAL files of your PSVita filesystem.

2) In ux0-/dump/, place the out folder that you got using vitadecrypt.

3) To rebuild ELF, on Windows run _RUNME.BAT.

4) After having rebuilded ELF, to rebuild SELF, on Windows run BATCH_MAKE_FSELF.BAT.

5) You can now use the ELFs in vitadecompiler, IDA, or radare2 or simply use an hexadecimal editor to look into them.

You can also hexedit as you want the ELFs then transform them into SELFs.

## WARNING : NEVER WRITE to your PSVita os0 nor vs0:.