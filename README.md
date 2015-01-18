<pre>
ezsh command [options]

context - contrl current context:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --set arg             set context var
  --echo arg            echo this params


copy - copy file or dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break

  *ouput - rules for output, "option output" for details
  *fileset - rules for file select, "option fileset" for details

cwebp - webp encode:
  --error arg (=2)                error attitude:
                                  0: ignore, 1: quiet, 2: report, 3: break
  -c [ --concurrency ]            concurrency encode with threads
  -q [ --quality ] arg (=50)      image quality
  -a [ --alphaQuality ] arg (=50) image alpha channel quality

  *ouput - rules for output, "option output" for details
  *fileset - rules for file select, "option fileset" for details

help - show help message:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --long                show long help
  --cmd arg             show only this command


list - list file or dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break

  *fileset - rules for file select, "option fileset" for details

mkdir - make dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  -p [ --parents ]      no error if existing, make parent directories as needed
  -d [ --dir ] arg      dirs to make


option - show option message:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --option arg          show only option


remove - remove file or dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break

  *fileset - rules for file select, "option fileset" for details

script - run file as ezsh script:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --dry                 dry run for debug
  -f [ --file ] arg     files to run
  --set arg             set var with value


sqliteImport - import file or dir to sqlite database:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --noBase              no base
  --db arg              sqlite database
  --table arg           table name of sqlite database
  -f [ --file ] arg     file or dir to import


start - start a program:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --exist arg           start exec, if this exist
  --existNot arg        start exec, if this not exist
  --fileExist arg       start exec, if this file exist
  --fileExistNot arg    start exec, if this file not exist
  --dirExist arg        start exec, if this dir exist
  --dirExistNot arg     start exec, if this dir not exist
  --stdOut arg          pipe stdout to this file


textpp - textpp file or dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  -f [ --force ]        ignore nonexistent files and arguments
  -D [ --define ] arg   define a macro

  *ouput - rules for output, "option output" for details
  *fileset - rules for file select, "option fileset" for details

unicode - unicode file or dir:
  --error arg (=2)      error attitude:
                        0: ignore, 1: quiet, 2: report, 3: break
  --bomAdd              add BOM in file header
  --bomRemove           remove BOM in file header
  --valid               check files utf8 encode

  *fileset - rules for file select, "option fileset" for details

</pre>

