#!/bin/bash

# eksetazw to plithos twn parametrwn
if [ "$#" -ne 4 ]; then
    echo "ERROR PARAMETERS"
    exit 1
fi

R_DIR=$1
TEXT_F=$2
WEB_NUM=$3
PAGE_NUM=$4

# eksetazw an uparxei to root_dir
if [ ! -d $R_DIR ]; then
    echo "ERROR DIR"
    exit 1
fi

# eksetazw an uparxei to text_file
if [ ! -f $TEXT_F ]; then
    echo "ERROR TEXT"
    exit 1
fi

# eksetazw an to w einai akeraios
if ! [[ $WEB_NUM =~ ^[0-9]+$ ]]; then
    echo "ERROR WEB_NUM NOT INT"
    exit 1
fi

# eksetazw an to p einai akeraios
if ! [[ $PAGE_NUM =~ ^[0-9]+$ ]]; then
    echo "ERROR PAGE_NUM NOT INT"
    exit 1
fi

# eksetazw an oi grammes >10000
LINES=$((`cat $TEXT_F | wc -l`))
if [ $LINES -lt 10000 ]; then
    echo "ERROR LINES < 10000"
    exit 1
fi

# eksetazw an to R_DIR einai adeio
if [ "$(ls -A $R_DIR)" ]; then
    echo "Warning: directory is full, purging ..."
    rm -rf $R_DIR/*
fi


# dhmiourgw ta tuxaia onomata twn pages
PAGE_NAME=()
SITE_NAME=()
for ((i=0; i<$WEB_NUM; i++)); do
    SITE_NAME+=( "site$i" )
    echo ${SITE_NAME[i]}
    for ((l=0; l<$PAGE_NUM; l++)); do
        FLAG=1
        while [[ $FLAG == 1 ]]; do
            FLAG=0
            j=$((1 + RANDOM % 50000))
            PAGE="$R_DIR/${SITE_NAME[i]}/page$i" 
            PAGE+="_$j.html"
            for p in "${PAGE_NAME[@]}"; do
                if [[ $p == $PAGE ]]; then
                    FLAG=1
                    break
                fi
            done
        done
        PAGE_NAME+=( $PAGE )
        echo ${PAGE_NAME[$l]}
    done
done

HAVE_OUT_LINK=1
# dhmiourgw ta sites kai ta pages tou kathe site
for ((i=0; i<$WEB_NUM; i++)); do
    echo "Creating web site $i ..."
    mkdir $R_DIR/${SITE_NAME[$i]}

    # dhmiourgw ta pages tou site
    for ((l=0; l<$PAGE_NUM; l++)); do
        # dhmiourgw to tuxaio k
        k=$((2 + RANDOM % "$(($LINES-2003))"))

        # dhmiourgw to tuxaio m
        m=$((1001 + RANDOM % 999))

        # dhmiourgw to f
        if [[ $PAGE_NUM == 1 ]]; then
            f=0
        elif [[ $PAGE_NUM == 2 ]]; then
            f=1
        else
            f=$(($(($PAGE_NUM / 2))+1))
        fi

        # dhmiourgw to q
        q=$(($(($WEB_NUM / 2))+1))

        # dhmiourgw to m
        LAST_LINE=$(($((m / $((f+q))))+$k))

        s=$(($(($i * $PAGE_NUM))+$l))
        echo "Creating page ${PAGE_NAME[$s]} with $(($LAST_LINE-$k)) lines starting at line $k ..."
        touch ${PAGE_NAME[$s]}

        echo -e "<!DOCTYPE html>" >> ${PAGE_NAME[$s]}
        echo -e "<html>" >> ${PAGE_NAME[$s]}
        echo -e "\t<body>" >> ${PAGE_NAME[$s]}
        echo -e "<pre>" >> ${PAGE_NAME[$s]}
        sed -n "$k","$LAST_LINE"p < $TEXT_F >> ${PAGE_NAME[$s]}

        # vriskw ta tuxaia eswterika links
        IN_RND_PAGES=( $(($(($i*$PAGE_NUM))+$l)) )
        for ((c=0; c<f; c++)); do
            FLAG=1
            while [[ $FLAG == 1 ]]; do
                FLAG=0
                IN_RND_PAGE_NUM=$(($(($i*$PAGE_NUM)) + RANDOM % "$PAGE_NUM"))
                for p in "${IN_RND_PAGES[@]}"; do
                    if [[ $p == $IN_RND_PAGE_NUM ]]; then
                        FLAG=1
                        break
                    fi
                done
            done
            IN_RND_PAGES+=( $IN_RND_PAGE_NUM )
            IN_LINK="<a href="../../${PAGE_NAME[$IN_RND_PAGE_NUM]}">link$c"
            IN_LINK+="_text</a>"
            echo "Adding link to ${PAGE_NAME[$IN_RND_PAGE_NUM]}"
            echo -e "$IN_LINK" >> ${PAGE_NAME[$s]}
        done

        # vriskw ta tuxaia ekswterika links
        OUT_RND_PAGES=()
        for ((c=0; c<q; c++)); do
            FLAG=1
            if (( $WEB_NUM == 1 )); then
                HAVE_OUT_LINK=0
                break
            fi
            while [[ $FLAG == 1 ]]; do
                FLAG=0
                OUT_RND_PAGE_NUM=$((RANDOM % "$(($WEB_NUM*$PAGE_NUM))"))
                if [[ ( $OUT_RND_PAGE_NUM -ge $(($i*$PAGE_NUM)) ) && ( $OUT_RND_PAGE_NUM -lt $(($(($i+1))*$PAGE_NUM)) ) ]]; then
                    FLAG=1
                fi
                for p in "${OUT_RND_PAGES[@]}"; do
                    if [[ $p == $OUT_RND_PAGE_NUM ]]; then
                        FLAG=1
                        break
                    fi
                done
            done
            OUT_RND_PAGES+=( $OUT_RND_PAGE_NUM )
            OUT_LINK="<a href="../../${PAGE_NAME[$OUT_RND_PAGE_NUM]}">link$(($c+$f))"
            OUT_LINK+="_text</a>"
            echo "Adding link to ${PAGE_NAME[$OUT_RND_PAGE_NUM]}"
            echo -e "$OUT_LINK" >> ${PAGE_NAME[$s]}
        done

        echo -e "</pre>" >> ${PAGE_NAME[$s]}
        echo -e "\t</body>" >> ${PAGE_NAME[$s]}
        echo -e "</html>" >> ${PAGE_NAME[$s]}
    done
done

if [[ $HAVE_OUT_LINK == 1 ]]; then
    echo "All pages have at least one incoming link"
else
    echo "Pages have not at least one incoming link"
fi

echo "Done."








