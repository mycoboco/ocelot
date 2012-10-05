#!/bin/bash
#
# Generates documents from code for ocelot (for internal use only)
#

# man-page number
#
MANPAGE=3

# directories under doc
#
DOCHTML="html"
DOCPDF="pdf"
DOCMAN="man${MANPAGE}"
DOCDIRS="${DOCHTML} ${DOCMAN} ${DOCMAN}/cbl ${DOCMAN}/cdsl ${DOCMAN}/cel ${DOCPDF}"

# directories for document sources
#
SRCDIRS="cbl/arena cbl/assert cbl/except cbl/memory cbl/text cdsl/dlist cdsl/hash cdsl/list \
         cdsl/stack cdsl/set cdsl/table cel/conf cel/opt"

# directories that doxygen generates
#
OUTHTML="html"
OUTLATEX="latex"
OUTMAN="man"
OUTMANPAGE="${OUTMAN}/man${MANPAGE}"
OUTDIRS="${OUTHTML} ${OUTLATEX} ${OUTMAN} ${OUTMANPAGE}"

# pdf file that doxygen generates
#
OUTPDFF="refman.pdf"

# doxygen configuration file
#
DOXYFILE="Doxyfile"

# document file that contains @version
#
DOXYMAIN="doxymain.h"

# files to exclude from man-pages
#
MANTRASH="\@*.3 ALIGNED.3 EQUAL.3 FREE_THRESHOLD.3 HASH.3 IDX.3 ISATEND.3 MULTIPLE.3 \
          NDESCRIPTORS.3 NELEMENT.3 RAISE_EXCEPT_IF_INVALID.3 SWAP.3 afile.3 afunc.3 aline.3 \
          asize.3 ifile.3 ifunc.3 iline.3 len.3 logfuncFreefree.3 logfuncResizefree.3 p.3 size.3 \
          str.3 todo.3 uintptr_t.3 NALLOC.3 BUFLEN.3 VALID_CHR.3 defval.3 type.3 var.3 MAX.3 MIN.3 \
          data.3 next UC.3 arg.3 flag.3 lopt.3 order.3 sopt.3"

fatal() {
    echo "$0: $1"
    exit 1
}

chdir() {
    cd $1 || fatal "failed to change pwd to $1"
}

echo "Deleting old documents if any..."
    rm -rf ${DOCDIRS}
    for dir in ${DOCDIRS}; do
        if [ -d "${dir}" ]; then
            fatal "failed to delete ${dir}"
        fi
    done
    for src in ${SRCDIRS}; do
        for out in ${OUTDIRS}; do
            rm -rf "../${src}/${out}"
            if [ -d "../${src}/${out}" ]; then
                fatal "failed to delete ../${src}/${out}"
            fi
        done
    done


echo "Creating sub-directories..."
    for dir in ${DOCDIRS}; do
        mkdir "${dir}" || fatal "failed to create ${dir}"
    done

chdir ".."
for dir in ${SRCDIRS}; do
    chdir "${dir}"
    echo -e "\nGenerating documents for ${dir}..."
        if [ ! -f "${DOXYFILE}" ]; then
            fatal "${dir}/${DOXYFILE} does not exist"
        fi
        echo "    Executing doxygen..."
            doxygen "${DOXYFILE}" > /dev/null || fatal "failed to execute doxygen"
            for subdir in ${OUTDIRS}; do
                if [ ! -d "${subdir}" ]; then
                    fatal "doxygen does not create ${subdir}"
                fi
            done

        echo "    Collecting necessary information..."
            PREFIX=`echo "${dir}" | sed 's/\//\./'`
            if [ "X${PREFIX}" = "X" ]; then
                fatal "failed to construct the prefix"
            fi

            MANDIR=`echo ${PREFIX} | sed 's/\./ /' | awk '{ print $1 }'`
            if [ "X${MANDIR}" = "X" ]; then
                fatal "failed to construct the man directory"
            fi

            VERSION=`grep "@version" "${DOXYMAIN}" | awk '{ print $3 }'`
            if [ "X${VERSION}" = "X" ]; then
                fatal "failed to get the version number"
            fi

        echo "    Processing generated html files..."
            HTMLDIR="${PREFIX}-${VERSION}.html"
            mv "${OUTHTML}" "${HTMLDIR}" || fatal "failed to rename '${OUTHTML}' in ${dir}"
            rm -f "${HTMLDIR}/installdox"
            tar cvzf "${HTMLDIR}.tar.gz" "${HTMLDIR}" > /dev/null || \
                fatal "failed to compress '${OUTHTML}' in ${dir}"
            rm -rf "${HTMLDIR}"
            if [ -d "${HTMLDIR}" ]; then fatal "failed to delete ${HTMLDIR}"; fi
            mv "${HTMLDIR}.tar.gz" "../../doc/${DOCHTML}/" || fatal "failed to move html file in ${dir}"

        echo "    Processing generated latex files..."
            PDFFILE="${PREFIX}-${VERSION}.pdf"
            chdir "${OUTLATEX}"
            make pdf > /dev/null 2>&1 || fatal "failed to create pdf file in ${dir}"
            mv "${OUTPDFF}" "${PDFFILE}" || fatal "failed to rename pdf file in ${dir}"
            mv "${PDFFILE}" ../../../doc/${DOCPDF}/ || fatal "failed to move pdf file in ${dir}"
            chdir ".."
            rm -rf "${OUTLATEX}"
            if [ -d "${OUTLATEX}" ]; then fatal "failed to delete '${OUTLATEX}' in ${dir}"; fi

        echo "    Processing generated man pages..."
            chdir "${OUTMANPAGE}"
            rm -f ${MANTRASH}
            mv * "../../../../doc/${DOCMAN}/${MANDIR}" || fatal "failed to move man-pages in ${dir}"
            chdir "../.."
            rm -rf "${OUTMAN}"
            if [ -d "${OUTMAN}" ]; then fatal "failed to delete 'man' in ${dir}"; fi

        chdir "../.."
done

echo -e "\nAll documents generated"

# end of gendoc.sh
