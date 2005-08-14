%define name	hocr
%define version	0.3.0
%define release 2

Name:		%{name} 
Summary:		hocr is a Hebrew OCR (optical character recognition) program
Version:		%{version}
Release: 		%mkrel %release 
# Don't forget to bzme the source package
Source0:		http://download.berlios.de/hocr/hocr-%{version}.tar.bz2
Source1:		hocr-sample-1.jpg
Source2:		hocr-sample-2.jpg
URL:			http://hocr.berlios.de

Group:		Graphical desktop/GNOME
BuildArch:		i586 
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-buildroot 
BuildRequires:	libgnomeuimm-devel
BuildRequires:	libgtkspell0-devel
Requires:		aspell-he
License:		GPL

Packager:		Dovix, dovix2003@yahoo.com

%description
The program will process black&white texts
scanned at 300dpi.
The text is read without punctuation.
Spell checking is using aspell-he.
Menu location: Office -> Accessories -> Hebrew OCR
Sample files: /usr/share/misc/hocr/

%description -l he_IL
התוכנה מעבדת תמונות jpeg שנסרקו ב 300 נקודות
לאינצ' שחור-לבן.
היא קוראת טקסט ללא ניקוד, ומסוגלת לבדוק את האיות
של הטקסט הנקרא בעזרת aspell-he.
מיקום בתפריט: משרד -> עזרים -> Hebrew OCR
קבצי דוגמה: /usr/share/misc/hocr/

%prep
%setup -q -a 0 -n %{name}-%{version}

%build
%configure
%make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall
%find_lang %{name}

install -d -m 755 %buildroot%_datadir/misc/%{name}
install -m 755 %{SOURCE1} %buildroot%_datadir/misc/%{name}
install -m 755 %{SOURCE2} %buildroot%_datadir/misc/%{name}

# Creating menu entry

install -d -m 755 $RPM_BUILD_ROOT%{_menudir}

cat >$RPM_BUILD_ROOT%{_menudir}/%{name} <<EOF
?package(%{name}): \
	command="%{_bindir}/hocr-gui" \
	icon="news_section.png" \
	needs="X11" \
	section="Office/Accessories" \
	title="Hebrew OCR" \
	longtitle="Hebrew optical character recognition program, for 300 dpi b/w jpg scans" 
EOF

%clean
rm -rf $RPM_BUILD_ROOT 

# Handling internationalization cleanly (use file list)
%files -f %{name}.lang
%defattr(-,root,root)
%{_bindir}/hocr-gui
%{_menudir}/%{name}
%{_datadir}/misc/%{name}/*
%doc README AUTHORS NEWS COPYING ChangeLog INSTALL

%post
%update_menus

%postun
%clean_menus

%changelog
* Mon Aug 15 2005 Dovix <dovix2003@yahoo.com> 0.3.0-1.1.102mdk
- Added menu entry
- Added Hebrew description
- Added sample files

* Sun Aug 14 2005 Dovix <dovix2003@yahoo.com> 0.3.0-0.1.102mdk
- First release