#!/bin/bash
# Pass keywords.txt data to keywords.c
keywords_file="libs/compiler/keywords.c"
keywords_source="src/keywords.txt"

# Back off if script is running not from project root
if [ ! -f "${keywords_file}" ] || [ ! -f "${keywords_source}" ] ; then
    echo "${keywords_source} or ${keywords_file} don't exist."
    echo "Is script called from project root?"
    exit 1
fi

# Now create the file
echo "char keywords_txt[] ="> "${keywords_file}"
first_appended=
cat "${keywords_source}" | while read line
do
    if [ "${first_appended}" == "" ] ; then
        first_appended=1
    else
        echo -n -e '\n' >> "${keywords_file}"
    fi
    echo -n "    \"$line\n\"" >> "${keywords_file}"
done
echo ";" >> "${keywords_file}"
