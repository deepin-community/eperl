# New ports collection makefile for:	p5-ePerl
# Version required:	2.2.10
# Date created:		03-Sep-1997
# Whom:	      		Ralf S. Engelschall <rse@engelschall.com>
#
# $Id: Makefile,v 1.11 1997/12/28 17:11:46 rse Exp $
#

DISTNAME=	eperl-2.2.10
PKGNAME=	p5-ePerl-2.2.10
CATEGORIES=	lang www perl5
MASTER_SITES=	${MASTER_SITE_PERL_CPAN} \
		http://www.engelschall.com/sw/eperl/distrib/ \
		ftp://ftp.engelschall.com/sw/eperl/ \
		ftp://ftp.hyperreal.org/software/engelschall/eperl/
MASTER_SITE_SUBDIR=Parse

MAINTAINER=	rse@engelschall.com

USE_PERL5=	YES

MAN3=		Parse::ePerl.3 Apache::ePerl.3
MANPREFIX=	${PREFIX}/lib/perl5

do-configure:
	@ cd ${WRKSRC}; ${SETENV} ${MAKE_ENV} ${PREFIX}/bin/perl Makefile.PL

.include <bsd.port.mk>
