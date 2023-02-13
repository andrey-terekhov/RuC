import sys, os

DEFAULT_FILES = [ "head.s", "foot.s" ]

def check_file(file):
  if not os.path.isfile(file):
    raise Exception(f"Wrong file argument {file}")
def process_file(file):
  i = open(file, 'r')
  e = os.path.splitext(file)
  o = open(f"{e[0]}.min{e[1]}", 'w')
  for l in i.readlines():
    s = l.split("#")[0]
    s = s.strip()
    if len(s) == 0:
      continue
    o.write(s + "\n")
  i.close()
  o.close()

def main():
  files = DEFAULT_FILES
  if len(sys.argv) > 1:
    files = sys.argv[1:]

  for f in files:
    check_file(f)

  for f in files:
    print("Processing", f)
    process_file(f)

if __name__ == "__main__":
  main()
