MainWindow.o: MainWindow.c MainWindow.h Settings.h SettingsDialog.h fileutils.h StoryReader.h StoryTerminal.h Interpreter.h dialogs.h
Settings.o: Settings.c Settings.h 
SettingsDialog.o: SettingsDialog.c SettingsDialog.h Settings.h kbcomboboxtext.h
main.o: main.c MainWindow.h 
fileutils.o: fileutils.c fileutils.h 
kbcomboboxtext.o: kbcomboboxtext.c kbcomboboxtext.h
StoryReader.o: StoryReader.c StoryReader.h ZMachine.h Picture.h blorbreader.h Sound.h
StoryTerminal.o: StoryTerminal.c StoryTerminal.h blorbreader.h colourutils.h charutils.h
Interpreter.o: Interpreter.c Interpreter.h
ZMachine.o: ZMachine.c ZMachine.h frotz.h Picture.h Sound.h MediaPlayer.h StoryReader.h Interpreter.h
blorbreader.o: blorbreader.c blorbreader.h Picture.h MetaData.h ZTerminal.h
Picture.o: Picture.c Picture.h
MetaData.o: MetaData.h MetaData.c
ZTerminal.o: ZTerminal.c ZTerminal.h StoryTerminal.h charutils.h frotz.h
charutils.o: charutils.c charutils.h
colourutils.o: colourutils.c colourutils.h
frotz_main.o: frotz_main.c frotz.h
frotz_buffer.o: frotz_buffer.c frotz.h
frotz_err.o: frotz_err.c frotz.h
frotz_process.o: frotz_process.c frotz.h
frotz_fastmem.o: frotz_fastmem.c frotz.h
frotz_files.o: frotz_files.c frotz.h
frotz_hotkey.o: frotz_hotkey.c frotz.h
frotz_input.o: frotz_math.c frotz.h
frotz_input.o: frotz_input.c frotz.h
frotz_math.o: frotz_math.c frotz.h
frotz_object.o: frotz_object.c frotz.h
frotz_quetzal.o: frotz_quetzal.c frotz.h
frotz_random.o: frotz_random.c frotz.h
frotz_redirect.o: frotz_redirect.c frotz.h
frotz_screen.o: frotz_screen.c frotz.h
frotz_sound.o: frotz_sound.c frotz.h
frotz_stream.o: frotz_stream.c frotz.h
frotz_table.o: frotz_table.c frotz.h
frotz_text.o: frotz_text.c frotz.h
frotz_variable.o: frotz_variable.c frotz.h
dialogs.o: dialogs.c dialogs.h
Sound.o: Sound.c Sound.h
MediaPlayer.o: MediaPlayer.h MediaPlayer.c
