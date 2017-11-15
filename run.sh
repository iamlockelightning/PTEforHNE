#!/bin/sh

text_file=../WikiExtractor/etc/en_text_all.txt # the text file for training
infer_file=../WikiExtractor/etc/zh_text_all.txt
wiki_path=../WikiExtractor/etc/
output_path=workspace/

window=5 # the window size for the construction of the word-word network
min_count=0 # discard words that appear less than <min_count>



# heterogeneous text network construction
# ./text2hin/data2w -text ${text_file} -output-ww ${output_path}en.ww.net -output-words ${output_path}en.words.node -window ${window} -min-count ${min_count}



# cat ${output_path}en.words.node ${output_path}zh.words.node > ${output_path}all.words.node



# learn predictive word representations
# ./pte/pte -nodes ${output_path}all.words.node -words ${output_path}all.words.node -enhin ${output_path}en.ww.net -zhhin ${output_path}zh.ww.net -clhin ${output_path}cl.train.40000.net -output ${output_path}word.emb -binary 0 -size 30 -negative 5 -samples 10000 -alpha 0.025 -lr 0.01 -MARGIN 1.0 -lambda 0.01 -threads 30
# cat ${output_path}all.words.node ${wiki_path}en_title_all.txt ${wiki_path}en_title_all.txt | sort | uniq > ${output_path}new.all.words.node
./pte/pte -nodes ${output_path}new.all.words.node -words ${output_path}new.all.words.node -enhin ${output_path}en.ww.net -zhhin ${output_path}zh.ww.net -enlinkhin ${output_path}en.linkage.net -zhlinkhin ${output_path}zh.linkage.net -clhin ${output_path}cl.train.40000.net -output ${output_path}1116.TL.transE.word.emb -binary 0 -size 30 -negative 5 -samples 1000 -alpha 0.025 -lr_1 0.001 -MARGIN 1.0 -lambda_1 0.1 -lr_2 0.001 -lambda_2 1.0 -threads 26


# infer the embeddings of the texts provided in the <infer_file>
# ./text2vec/infer -infer ${infer_file} -vector ${output_path}word.emb -output ${output_path}text.emb -debug 2 -binary 0



# ./utils/combine -title ${wiki_path}zh_title_all.txt -vector ${output_path}zh.text.emb -output ${output_path}zh.title.text.emb
# ./utils/combine -title ${wiki_path}en_title_all.txt -vector ${output_path}en.text.emb -output ${output_path}en.title.text.emb
# sed -i '1d' ${output_path}zh.title.text.emb
# cat ${output_path}en.title.text.emb ${output_path}zh.title.text.emb > ${output_path}all.title.text.emb # modify num



# ./utils/rank -input ${output_path}all.title.text.emb -pair ${output_path}cl.train.40000.net
