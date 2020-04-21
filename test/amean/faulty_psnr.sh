echo "running raul.cfg with fault enabled and calculating PSNR..."
make run 
python ../../../utility/psnr.py amean_outimg_0.ch amean_outimg_golden_16.ch