ezsh command [options]

context - contrl current context:
  --set arg             set context var
  --echo arg            echo this params


cwebp - webp encode:
  -c [ --concurrency ]            concurrency encode with threads
  -i [ --input ] arg              file or dir to encode
  -o [ --output ] arg             file or dir for output
  -q [ --quality ] arg (=50)      image quality
  -a [ --alphaQuality ] arg (=50) image alpha channel quality


help - show help message:
  --long                show long help
  --cmd arg             show only this command


list - list file or dir:
  --noError             do not report error

  fileset - rules for file select, "option fileset" for details



mkdir - make dir:
  -p [ --parents ]      no error if existing, make parent directories as needed
  -d [ --dir ] arg      dirs to make


option - show option message:
  --option arg          show only option


remove - remove file or dir:
  -f [ --force ]        ignore nonexistent files and arguments

  fileset - rules for file select, "option fileset" for details



script - run file as ezsh script:
  --dry                 dry run for debug
  -f [ --file ] arg     files to run
  --set arg             set var with value


sqliteImport - import file or dir to sqlite database:
  --noBase              no base
  --db arg              sqlite database
  --table arg           table name of sqlite database
  -f [ --file ] arg     file or dir to import


start - start a program:
  --exist arg           start exec, if this exist
  --existNot arg        start exec, if this not exist
  --fileExist arg       start exec, if this file exist
  --fileExistNot arg    start exec, if this file not exist
  --dirExist arg        start exec, if this dir exist
  --dirExistNot arg     start exec, if this dir not exist
  --stdOut arg          pipe stdout to this file


textpp - textpp file or dir:
  -f [ --force ]        ignore nonexistent files and arguments
  -D [ --define ] arg   define a macro

  ouput - rules for output, "option output" for details


  fileset - rules for file select, "option fileset" for details



unicode - unicode file or dir:
  --bomAdd              add BOM in file header
  --bomRemove           remove BOM in file header
  --valid               check files utf8 encode

  fileset - rules for file select, "option fileset" for details



