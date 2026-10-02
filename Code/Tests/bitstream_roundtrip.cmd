@rem ===========================================================================
@rem SCAFFOLDING - NOT RECOVERED SOURCE. NOT PART OF THE BF2 BUILD.
@rem
@rem Empirically verifies the recovered BitStream primitives:
@rem   * writeBits / readBits                    - exact, bit for bit
@rem   * writeCompressedVector     / readCompressedVector     - within one quantum (lossy)
@rem   * writeHighCompressedVector / readHighCompressedVector - same, over the other table
@rem   * writeCompressedVector2    / readCompressedVector2    - 3-bit selector + zero sentinel
@rem   * writeNormalVector         / readNormalVector         - by dot product and unit length
@rem   * writeUnitQuaternion3Comp  / readUnitQuaternion3Comp  - by |dot|; NOT always unit length
@rem   * writeUnitQuaternion       / readUnitQuaternion       - by |dot|; reader renormalises -> unit
@rem   * shrinkNormalVector                                   - unit length + quantised direction
@rem The two were decoded SEPARATELY from bf2.i64; this checks they are actually
@rem inverse rather than merely looking inverse.
@rem
@rem Unlike syntaxcheck.cmd (which is /Zs, parse-only), this one COMPILES AND RUNS.
@rem It is the only executable verification in the tree.
@rem
@rem Last run:
@rem   quat cases:   1800  failures: 0  worst |dot|: 0.993884
@rem   quat4 cases:  1800  failures: 0  worst |dot|: 0.999913  (full 4-comp; renormalising reader)
@rem   shrink cases: 1500  failures: 0  worst dot: 0.999488    (pure-math normal preview)
@rem   normal cases: 1800  failures: 0  worst dot: 0.99953   (~1.75 deg at 8 bits)
@rem   v2 cases:     2000  failures: 0  selector coverage: 0=376 1=229 2=268 3=249 4=122 5=114 6=442 7=200
@rem   vector cases: 4000  failures: 0  selector coverage: 0=727 1=468 2=677 3=2128
@rem   round-trip cases: 2048  failures: 0  guard failures: 0
@rem
@rem THE SELECTOR HISTOGRAM IS NOT DECORATION - it proves every encoding path actually ran.
@rem It has already earned its place: the first CompressedVector2 sweep reported 0 failures
@rem while showing 0=0 and 1=0, i.e. the uncompressed path and the 24-bit tier were never
@rem exercised at all. The random span only reached 1<<16, short of table2[1]=24 bits. Widened
@rem to 1<<30 and both now run. A pass rate alone would have hidden that entirely.
@rem
@rem The normal sweep deliberately includes AXIS-ALIGNED inputs (+Z and +X), because those are what
@rem exercise the decoder-only snap-to-zero dead zone. A sweep of random directions alone would
@rem never hit it, and the dead zone is the one part of that pair the encoder gives no hint of.
@rem
@rem The quaternion tolerance is deliberately LOOSE ON LENGTH. The decoder clamps x^2+y^2+z^2 to 1
@rem before the sqrt, so when quantisation overshoots, w becomes 0 and |q| ends up above 1 - about
@rem 1% at 8 bits. That is BF2's real behaviour and the test must accept it. A strict unit-length
@rem assertion here fails 9 of 1800 cases for a correct implementation, which is how the behaviour
@rem was found in the first place.
@rem
@rem The v2 sentinel check is also specific: selector 7 must cost EXACTLY 3 bits and reproduce
@rem the reference exactly, so a rebuild that silently wrote components anyway would fail here.
@rem ===========================================================================
@echo off
setlocal
call "%~dp0..\Build\vsdevcmd.cmd" x86
pushd "%~dp0"
cl /nologo /EHsc /O2 /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" bitstream_roundtrip.cpp
if errorlevel 1 ( popd & exit /b 1 )
"%~dp0bitstream_roundtrip.exe"
set RC=%errorlevel%
popd
exit /b %RC%
