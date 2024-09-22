# Run this script before commiting

git ls-files | grep -e ".*\.\(c\|h\|hpp\|cc\|cpp\|hh\)\$" | grep -vf .clang-format-ignore | xargs clang-format -i -style=file --verbose