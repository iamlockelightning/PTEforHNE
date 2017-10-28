#!/bin/sh

text_file=../WikiExtractor/etc/en_text_all.txt # the text file for training
output_path=workspace/

window=5 # the window size for the construction of the word-word network
min_count=0 # discard words that appear less than <min_count>



# heterogeneous text network construction
# ./text2hin/data2w -text ${text_file} -output-ww ${output_path}en.ww.net -output-words ${output_path}en.words.node -window ${window} -min-count ${min_count}



# cat ${output_path}en.words.node ${output_path}zh.words.node > ${output_path}all.words.node



# learn predictive word representations
./pte/pte -nodes ${output_path}all.words.node -words ${output_path}all.words.node -enhin ${output_path}en.ww.net -zhhin ${output_path}zh.ww.net -clhin ${output_path}cl.train.40000.net -output ${output_path}word.emb -binary 0 -size 30 -negative 5 -samples 10000 -alpha 0.025 -lr 0.01 -MARGIN 1.0 -lambda 0.01 -threads 30



# infer the embeddings of the texts provided in the <infer_file>
# ./text2vec/infer -infer ${infer_file} -vector ${output_path}word.emb -output ${output_path}text.emb -debug 2 -binary 0



# ./utils/combine -title ../WikiExtractor/etc/zh_title_40000_all.txt -vector ${output_path}zh.text.emb -output ${output_path}zh.title.text.emb
# ./combine -title ../WikiExtractor/etc/en_title_40000_all.txt -vector ${output_path}en.text.emb -output ${output_path}en.title.text.emb
# cat ${output_path}en.title.text.emb ${output_path}zh.title.text.emb > ${output_path}all.title.text.emb # modify num



# ./utils/rank -input ${output_path}all.title.text.emb -pair ${output_path}cl.train.40000.net
