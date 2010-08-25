#!/usr/bin/env python
# vim: set fileencoding=utf-8 :
# Andre Anjos <andre.anjos@idiap.ch>
# Wed 25 Aug 2010 17:28:44 CEST 

"""This program allows you to play with Torch5spro image filters. To get
started, list all the filters available with the following command:

  $ filter.py list

To get specialized help on a particular filter and options just do:

  $ filter.py crop --help
"""

import sys, os
from torch.filters import FILTERS

def handle_filter(f, args):
  """Handles the processing of a certain filter. If you are curious about which
  filters are available, please look inside the torch.filters module"""

  import optparse
  parser = optparse.OptionParser(prog="%s %s" % \
      (os.path.basename(sys.argv[0]), f.__name__.lower()),
      usage='%prog [options] ' + ' '.join(f.arguments),
      description=f.__doc__)
  for k in f.options: parser.add_option(*k[0], **k[1])
  options, args = parser.parse_args()

  if len(args) != (len(f.arguments) + 1):
    parser.error("this program requires %d positional argument(s)" % \
        len(f.arguments))
  
  #finally, we call the filter with the given parametrization
  f()(options, args[1:])

def format_doc(d, width=80, prefix='           | '):
  """Formats the documentation given to fit in the number of columns
  defined."""
  cols = width - len(prefix)
  curline = ''
  lines = []
  for k in d.split():
    if len(curline) <= cols:
      if not curline: 
        curline += k
        continue
      if len(' '.join([curline, k])) <= cols: 
        curline += ' ' + k
      else:
        if not lines: lines.append(curline)
        else: lines.append(prefix + curline)
        curline = k 
  lines.append(prefix + curline)
  return '\n'.join(lines)

def main():

  if len(sys.argv) == 1:
    print __doc__
    sys.exit(1)

  elif len(sys.argv) == 2:
    if sys.argv[1].lower() in ('--help', '-h', 'help', '-?'):
      print __doc__
      sys.exit(1)
    elif sys.argv[1].lower() in ('list', '-l'):
      width = 80 #number of terminal columns
      for k in FILTERS:
        print ' %-9s | Descrition' % 'Filter'
        print '-----------+%s' % ((width-12)*'-',)
        print '%-10s | %s' % (k.__name__.lower(), format_doc(k.doc, width))
      sys.exit(1)
    elif sys.argv[1].lower() in [k.__name__.lower() for k in FILTERS]:
      filter = [k for k in FILTERS if k.__name__.lower() ==
          sys.argv[1].lower()][0]
      handle_filter(filter, sys.argv[3:])
    else:
      print 'ERROR: I do not understand command "%s"' % sys.argv[1]
      print 'For a list of known filters, type "list"'
      sys.exit(1)

  else:
    if sys.argv[1].lower() in [k.__name__.lower() for k in FILTERS]:
      filter = [k for k in FILTERS if k.__name__.lower() ==
          sys.argv[1].lower()][0]
      handle_filter(filter, sys.argv[2:])
    else:
      print __doc__
      sys.exit(1)

if __name__ == '__main__':
  main()

