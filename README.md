# Labor Betriebssysteme

Dies ist das Template für die Aufgaben zum Labor Betriebsysteme. Details zur Aufgabenstellung finden Sie im ILIAS-Kurs 
zum Labor.

Wenn die notwendige Arbeitsumgebung eingerichtet wurde, sollte sich das Template-Projekt korrekt übersetzen lassen und 
dann die Funktionalität des 
[_Simple & Stupid File Systems_](http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/) 
bereitstellen. Das lässt sich mit den folgenden Kommandos ausprobieren (dabei 
`<pfad-zum-dateisystem>` durch das Verzeichnis mit dem Template ersetzen):

	cmake CMakeLists.txt
	make
	mkdir -p mount
	bin/mount.myfs mount -l log.txt

