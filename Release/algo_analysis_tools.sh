#!/usr/bin/bash
export LANG="zh_CN.UTF-8"

local_log_file="../LogAlgoCenter/AlgoCenter_9090.INFO"

is_get_ratio=false
is_get_diversity=false
def_top_n=10
recall_type=""
def_exp_id=-1
def_api_type="\"apiType\":"
def_api_type_RT="("
def_user_id="\"user_id\":"
def_user_id_RT=", user_id = "
def_diversity_top=10
def_exp_layer="recall"
def_user_group=""

for pm in "$@"; do
    pms=(${pm//=/ })
    case "${pms[0]}" in
    '-r')
        is_get_ratio=true
        ;;
    '-d')
        is_get_diversity=true
        ;;
    '--log_dt')
        local_log_file=${local_log_file}"."${pms[1]}
        ;;
    '--top_n')
        def_top_n=${pms[1]}
        ;;
    '--recall_type')
        recall_type=${pms[1]}
        ;;
    '--exp_id')
        def_exp_id=${pms[1]}
        ;;
    '--api_type')
        def_api_type=${def_api_type}${pms[1]}
        def_api_type_RT=${def_api_type_RT}${pms[1]}")"
        ;;
    '--user_id')
        def_user_id=${def_user_id}${pms[1]}
        def_user_id_RT=${def_user_id_RT}${pms[1]}
        ;;
    '--diversity_top')
        def_diversity_top=${pms[1]}
        ;;
    '--exp_layer')
        def_exp_layer=${pms[1]}
        ;;
    '--user_group')
        def_user_group=${pms[1]}
        ;;
    '--help')
        echo "exe aat full name: algo analysis tools"
        echo "Usage: aat [OPTION]..."
        echo "   or: aat [OPTION]... --top_n=10"
        echo "Short options:"
        echo "    -r                        recall ratio 召回占比"
        echo "    -d                        recall diversity 召回多样性"
        echo "Long options:"
        echo "    --log_dt=YYYYMMDD_HH      使用指定日期时，默认当前日期时，例子：--log_dt=20230605_20"
        echo "    --top_n                   设置 top n，默认top 3，例子：--top_n=10"
        echo "    --recall_type             召回漏斗分析，例子：--recall_type=ResType_User_CF"
        echo "    --exp_id                  内部实验id，例子：--exp_id=10"
        echo "    --api_type                指定api类型，例子：--api_type=908001"
        echo "    --user_id                     指定用户user_id，例子：--user_id=177321497"
        echo "    --diversity_top           指定分析多样性 diversity top，默认top 10，例子：--diversity_top=10"
        echo "    --exp_layer               指定实验层，默认：recall，例子：--exp_layer=recall; 召回-recall 过滤-filter 展控-display"
        echo "    --user_group              指定用户组，例子：--user_group=def_group"
        echo "    --help                    display this help and exit"
        exit 0
    esac
done

use_exp_layer=",\"recallExpID\":"
if [ "X ${def_exp_layer}" == "X filter" ]; then
    use_exp_layer=",\"filterExpID\":"
elif [ "X ${def_exp_layer}" == "X display" ]; then
    use_exp_layer=",\"displayExpID\":"
fi

if [ ${def_exp_id} -ge 0 ]; then
    use_exp_layer=${use_exp_layer}${def_exp_id}
fi

use_user_group=",\"recallUserGroup\":"
if [ "X ${def_exp_layer}" == "X filter" ]; then
    use_user_group=",\"filterUserGroup\":"
elif [ "X ${def_exp_layer}" == "X display" ]; then
    use_user_group=",\"displayUserGroup\":"
fi

if [ "X ${def_user_group}" != "X " ]; then
    use_user_group=${use_user_group}"\"${def_user_group}\""
fi

if ${is_get_ratio}; then
    echo "统计召回占比..."
    grep "DebugLogFeature() Item Info Log:" ${local_log_file} | grep ${def_api_type} | grep ${use_exp_layer} | grep ${use_user_group} | grep ${def_user_id} | sed 's/^.*item_list\":\[{\(.*\)\}]}$/\1/g' | awk -F'},{' -v top_n=${def_top_n} '{for(i=1;i<=top_n&&i<=NF;i++){split($i,ary,",");dict_rt[substr(ary[2],15)]++}}END{for(k in dict_rt) printf("%-40s%s\n",k,"\t"dict_rt[k])}' | awk -F"\t" '{dict[$1]+=$2;total+=$2}END{for(k in dict){printf("%s%-10s%-20s%s\n",k,dict[k],dict[k]/total,total)}}' | sort -k2,2nr | awk 'BEGIN{printf("%-36s%-6s%-18s%s\n","召回类型","推荐次数","占比","总推荐次数")}{print $0}'
    echo ""
fi

if ${is_get_diversity}; then
    echo "统计推荐多样性..."
    grep "DebugLogFeature() Item Info Log:" ${local_log_file} | grep ${def_api_type} | grep ${use_exp_layer} | grep ${use_user_group} | grep ${def_user_id} | sed 's/^.*item_list\":\[{\(.*\)\}]}$/\1/g' | awk -F'},{' -v top_n=${def_top_n} '{for(i=1;i<=top_n&&i<=NF;i++){split($i,ary,",");dict_id[substr(ary[1],11)]++;}}END{for(k in dict_id) printf("%-30s%-10s\n",k,"\t"dict_id[k])}' | awk -F"\t" '{dict[$1]+=$2;total+=$2}END{for(k in dict)printf("%s%-10s%-20s%s\n",k,dict[k],dict[k]/total,"\t"total"\t"dict[k])}' | sort -k2,2nr | head -n ${def_diversity_top} | awk -F'\t' '{topnl[cnt++]=$1$2;total=$2;tp+=$3}END{printf("%-26s%-6s%-18s%s\n","推荐ID","推荐次数","占比","总推荐次数");for(k=0;k<cnt;k++){print topnl[k]}printf("%-30s%-10s%-20s%s\n","total",tp,tp/total,total)}'
    echo ""
fi

if [ "X ${recall_type}" != "X " ]; then
    echo "召回通路分析："${recall_type}

    use_exp_layer_RT=", recallExpID = "
    if [ "X ${def_exp_layer}" == "X filter" ]; then
        use_exp_layer_RT=", filterExpID = "
    elif [ "X ${def_exp_layer}" == "X display" ]; then
        use_exp_layer_RT=", displayExpID = "
    fi

    if [ ${def_exp_id} -ge 0 ]; then
        use_exp_layer_RT=${use_exp_layer_RT}${def_exp_id}
    fi

    use_user_group_RT=", recallUserGroup = "
    if [ "X ${def_exp_layer}" == "X filter" ]; then
        use_user_group_RT=", filterUserGroup = "
    elif [ "X ${def_exp_layer}" == "X display" ]; then
        use_user_group_RT=", displayUserGroup = "
    fi

    if [ "X ${def_user_group}" != "X " ]; then
        use_user_group_RT=${use_user_group_RT}${def_user_group}
    fi

    use_recall_type="recallType = ${recall_type}, "

    count=$(grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'Recall Info Log,' | wc -l)
    part_count=$(grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'Recall Info Log,' | awk -F'Item Num = ' '{if($2>0)part_cnt++}END{printf("%d",part_cnt)}')

    if [ ${count} -gt 0 ] && [ ${part_count} -gt 0 ]; then
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'Recall Info Log,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Recall:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Merge,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Merge:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Filter,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Filter:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Rank top50,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Rank top50:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Rank top20,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Rank top20:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Rank top8,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Rank top8:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Rank top3,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Rank top3:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Display top20,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Display top20:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Display top8,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Display top8:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
        grep -E "${use_recall_type}" ${local_log_file} | grep "${def_api_type_RT}" | grep "${use_exp_layer_RT}" | grep "${use_user_group_RT}" | grep "${def_user_id_RT}" | grep 'After Display top3,' | awk -F'Item Num = ' -v cnt=${count} -v part_cnt=${part_count} '{total+=$2}END{printf "%-30s%-20s%-20s%-20s%-20s%-20s\n","After Display top3:","total = "total,"cnt = "cnt,"avg = "total/cnt,"part_cnt = "part_cnt,"part_avg = "total/part_cnt}'
    fi
    echo ""
fi

