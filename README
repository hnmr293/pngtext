# What is this?

minimal tool for PNG `tEXt` chunk operations.

# Examples

## copy `tEXt` content to another file

copy a `tEXt` content of `source.png` to `base.png` and save new PNG file to `dest.png`.

```
$ cat source.png | ./pngtext -x | ./pngtext -o base.png dest.png
```

## delete `tEXt` content

delete a `tEXt` content of `input.png` and save new PNG file to `output.png`.

```
$ printf '' | ./pngtext -o input.png output.png
```

## replace `tEXt` content

replace a `tEXt` content with `sed`.

```
$ ./pngtext -x original.png | sed -e 's/xxx/yyy/g' | ./pngtext -o original.png output.png
```

# Usage

## Extract `tEXt` Chunk

```
$ pngtext -x
$ pngtext -x <input>
```

`pngtext -x` extracts a `tEXt` chunk content and dumps it to `stdout`.
If `<input>` is not specified, `stdin` is used as the input.

## Overwrite `tEXt` Chunk

```
$ pngtext -o <input> <output>
```

`pngtext -o` overwrites a `tEXt` chunk of the `<input>` file by texts from `stdin`. New PNG file is saved to `<output>` file.

## Compute CRC32

```
$ pngtext -c
```

`pngtext -c` computes CRC32 value of texts from `stdin`, and dumps it to `stdout` as hex string.

# How to compile

```
$ make
```
