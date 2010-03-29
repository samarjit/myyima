/*
 *
 *	This is free software. You can redistribute it and/or modify under
 *	the terms of the GNU Library General Public License version 2.
 * 
 *	Copyright (C) 1999 by kra
 *
 */
#ifndef __QP_NS_H__
#define __QP_NS_H__

// We are trying to recognize compilers that don't support namespaces.
// I think that number of compilers that support the feature will
// increase and old compilers will die.

// We are optimists.
#define __SUPPORT_NAMESPACES
// SUN Pro 4.2
#if defined( __SUNPRO_CC ) && __SUNPRO_CC <= 0x420
  #undef __SUPPORT_NAMESPACES
#endif

#if defined( __SUPPORT_NAMESPACES )

#define __BEGIN_QPTHR_NAMESPACE      namespace qpthr { using namespace std;
#define __BEGIN_QPTHR_NAMESPACE_PURE namespace qpthr {
#define __END_QPTHR_NAMESPACE        }  // namespace qpthr
#define __USE_QPTHR_NAMESPACE        using namespace qpthr;

#else

#define __BEGIN_QPTHR_NAMESPACE
#define __BEGIN_QPTHR_NAMESPACE_PURE
#define __END_QPTHR_NAMESPACE
#define __USE_QPTHR_NAMESPACE
  
#endif // __SUPPORT_NAMESPACES
 
#endif // __QP_NS_H__ 

