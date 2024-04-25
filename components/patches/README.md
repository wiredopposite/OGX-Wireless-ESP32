To apply the patches, do:

```
cd components/btstack
git apply ../patches/*.patch
```

And after applying the patch, you have to install btstack. E.g:

```
cd port/esp32
./integrate_btstack.py
```
