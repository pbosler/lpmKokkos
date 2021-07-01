# update_version_info.sh
# This script generates lpm/lpm_version.cpp in the build directory, which
# contains version information important for provenance.
# usage: bash update_version_info.sh source_root_dir dest_dir/haero_version.cpp

# Search source_root_dir/CMakeLists.txt for version information.
major_version=`grep '(LPM_VERSION_MAJOR' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/LPM_VERSION_MAJOR //'`
minor_version=`grep '(LPM_VERSION_MINOR' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/LPM_VERSION_MINOR //'`
patch_version=`grep '(LPM_VERSION_PATCH' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/LPM_VERSION_PATCH //'`

# check for git
which_git_wc=`which git | wc -l`

if [ $which_git_wc -gt 0 ]; then
  git_revision=`git log -1 --format=format:%h`
  git_diffs=`git diff | wc -l`
else
  git_revision="unknown"
  git_diffs=0
fi

if [ -f $2 ]; then
  cpp_file=$2.tmp
else
  cpp_file=$2
fi

cat > $cpp_file <<- END
// This file is automatically generated by update_version_info.sh.
// DO NOT EDIT!

namespace {
END

echo "// Version string" >> $cpp_file
echo "const char* _version = \"$major_version.$minor_version.$patch_version\";" >> $cpp_file
echo "// Git revision (hash) string" >> $cpp_file
echo "const char* _revision = \"$git_revision\";" >> $cpp_file
echo "// True iff there are uncomitted changes in your workspace" >> $cpp_file
if [ $git_diffs -gt 0 ]; then
  echo "bool _has_uncommitted_changes = true;" >> $cpp_file
else
  echo "bool _has_uncommitted_changes = false;" >> $cpp_file
fi
echo "} // anonymous namespace" >> $cpp_file

cat >> $cpp_file <<- END
namespace Lpm {

const char* version() { return _version; }

const char* revision() { return _revision; }

bool has_uncomitted_changes() { return _has_uncommitted_changes; }

} // namespace Lpm

END

# if we didn't generate the file in place, compare diffs and move if needed
if [ "$cpp_file" != $2 ] && [ -f $2 ]; then
  tmp_diff_wc=`diff $2 $cpp_file | wc -l`

  # if the files differ, move the temporary file into place; otherwise discard it
  if [ $tmp_diff_wc -gt 0 ]; then
    mv $cpp_file $2
  else
    rm -f $cpp_file
  fi
fi