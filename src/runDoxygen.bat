mkdir C:\!Out
mkdir C:\!Out\help
mkdir C:\!Out\help\html
del /S /Q C:\!Out\help\html\*
"C:\Program Files\doxygen\bin\doxygen.exe" FlylinkDC.doxygen
"C:\Program Files\HTML Help Workshop\hhc.exe" C:\!Out\help\html\index.hhp
