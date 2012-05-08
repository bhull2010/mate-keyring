/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-trust.c: Test XDG trust objects.

   Copyright (C) 2010 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#include "config.h"

#include "test-xdg-module.h"

#include "gkm/gkm-module.h"
#include "gkm/gkm-session.h"

#include "pkcs11/pkcs11.h"
#include "pkcs11/pkcs11n.h"
#include "pkcs11/pkcs11x.h"

static GkmModule *module = NULL;
static GkmSession *session = NULL;
static gpointer cert_data = NULL;
static gsize n_cert_data;

/*
 * C=ZA, ST=Western Cape, L=Cape Town, O=Thawte Consulting, OU=Certification Services Division,
 * CN=Thawte Personal Premium CA/emailAddress=personal-premium@thawte.com
 */

static const char DER_ISSUER[] =
	"\x30\x81\xCF\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x5A\x41"
	"\x31\x15\x30\x13\x06\x03\x55\x04\x08\x13\x0C\x57\x65\x73\x74\x65"
	"\x72\x6E\x20\x43\x61\x70\x65\x31\x12\x30\x10\x06\x03\x55\x04\x07"
	"\x13\x09\x43\x61\x70\x65\x20\x54\x6F\x77\x6E\x31\x1A\x30\x18\x06"
	"\x03\x55\x04\x0A\x13\x11\x54\x68\x61\x77\x74\x65\x20\x43\x6F\x6E"
	"\x73\x75\x6C\x74\x69\x6E\x67\x31\x28\x30\x26\x06\x03\x55\x04\x0B"
	"\x13\x1F\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74\x69\x6F\x6E\x20"
	"\x53\x65\x72\x76\x69\x63\x65\x73\x20\x44\x69\x76\x69\x73\x69\x6F"
	"\x6E\x31\x23\x30\x21\x06\x03\x55\x04\x03\x13\x1A\x54\x68\x61\x77"
	"\x74\x65\x20\x50\x65\x72\x73\x6F\x6E\x61\x6C\x20\x50\x72\x65\x6D"
	"\x69\x75\x6D\x20\x43\x41\x31\x2A\x30\x28\x06\x09\x2A\x86\x48\x86"
	"\xF7\x0D\x01\x09\x01\x16\x1B\x70\x65\x72\x73\x6F\x6E\x61\x6C\x2D"
	"\x70\x72\x65\x6D\x69\x75\x6D\x40\x74\x68\x61\x77\x74\x65\x2E\x63"
	"\x6F\x6D";

static const char SHA1_CHECKSUM[] =
	"\x36\x86\x35\x63\xfd\x51\x28\xc7\xbe\xa6\xf0\x05\xcf\xe9\xb4\x36"
	"\x68\x08\x6c\xce";

static const char MD5_CHECKSUM[] =
	"\x3a\xb2\xde\x22\x9a\x20\x93\x49\xf9\xed\xc8\xd2\x8a\xe7\x68\x0d";

static const char SERIAL_NUMBER[] =
	"\x01\x02\x03";

#define XL(x) G_N_ELEMENTS (x) - 1

#if 0

#include "egg/egg-asn1x.h"
#include "egg/egg-asn1-defs.h"
#include "egg/egg-hex.h"

static void
debug_print_certificate_info (const gchar *path)
{
	gchar *contents;
	gchar *results;
	gconstpointer data;
	gsize length;
	GNode *asn;

	if (!g_file_get_contents (path, &contents, &length, NULL))
		g_assert_not_reached ();

	results = g_compute_checksum_for_data (G_CHECKSUM_SHA1, (gpointer)contents, length);
	g_assert (results);
	g_printerr ("SHA1: %s\n", results);
	g_free (results);

	results = g_compute_checksum_for_data (G_CHECKSUM_MD5, (gpointer)contents, length);
	g_assert (results);
	g_printerr ("MD5: %s\n", results);
	g_free (results);

	asn = egg_asn1x_create_and_decode (pkix_asn1_tab, "Certificate", contents, length);
	g_assert (asn);

	data = egg_asn1x_get_raw_element (egg_asn1x_node (asn, "tbsCertificate", "issuer", NULL), &length);
	g_assert (data);

	results = egg_hex_encode_full (data, length, TRUE, '\\', 1);
	g_printerr ("ISSUER: %s\n", results);
	g_free (results);

	egg_asn1x_destroy (asn);
	g_free (contents);
}

#endif

TESTING_SETUP (trust_setup)
{
	CK_RV rv;

	module = test_xdg_module_initialize_and_enter ();
	session = test_xdg_module_open_session (TRUE);

	rv = gkm_module_C_Login (module, gkm_session_get_handle (session), CKU_USER, NULL, 0);
	g_assert (rv == CKR_OK);

	cert_data = testing_data_read ("test-certificate-2.cer", &n_cert_data);
	g_assert (cert_data);
}

TESTING_TEARDOWN (trust_teardown)
{
	test_xdg_module_leave_and_finalize ();
	module = NULL;
	session = NULL;

	g_free (cert_data);
	cert_data = NULL;
	n_cert_data = 0;
}

TESTING_TEST (trust_load_objects)
{
	CK_OBJECT_CLASS klass = CKO_NETSCAPE_TRUST;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
	};

	CK_ULONG n_objects;
	CK_OBJECT_HANDLE objects[16];
	CK_RV rv;

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	g_assert (rv == CKR_OK);
	rv = gkm_session_C_FindObjects (session, objects, G_N_ELEMENTS (objects), &n_objects);
	g_assert (rv == CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	g_assert (rv == CKR_OK);

	gkm_assert_cmpulong (n_objects, >=, 1);
}

TESTING_TEST (trust_create_assertion_complete)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_ANCHORED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE check = 0;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, &check, 1, &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_objects, ==, 1);
	gkm_assert_cmpulong (check, ==, object);
}

TESTING_TEST (trust_complete_assertion_has_no_serial_or_issuer)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_ANCHORED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_ATTRIBUTE check;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	check.type = CKA_SERIAL_NUMBER;
	check.pValue = NULL;
	check.ulValueLen = 0;
	rv = gkm_session_C_GetAttributeValue (session, object, &check, 1);
	gkm_assert_cmprv (rv, ==, CKR_ATTRIBUTE_TYPE_INVALID);

	check.type = CKA_ISSUER;
	check.pValue = NULL;
	check.ulValueLen = 0;
	rv = gkm_session_C_GetAttributeValue (session, object, &check, 1);
	gkm_assert_cmprv (rv, ==, CKR_ATTRIBUTE_TYPE_INVALID);
}

TESTING_TEST (trust_complete_assertion_netscape_md5_hash)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_OBJECT_CLASS nklass = CKO_NETSCAPE_TRUST;
	CK_X_ASSERTION_TYPE atype = CKT_X_PINNED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE check = 0;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	CK_ATTRIBUTE lookup[] = {
		{ CKA_CERT_MD5_HASH, (void*)MD5_CHECKSUM, XL (MD5_CHECKSUM) },
		{ CKA_CLASS, &nklass, sizeof (nklass) },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, lookup, G_N_ELEMENTS (lookup));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, &check, 1, &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (check, !=, 0);
	gkm_assert_cmpulong (n_objects, >, 0);
}

TESTING_TEST (trust_complete_assertion_netscape_sha1_hash)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_OBJECT_CLASS nklass = CKO_NETSCAPE_TRUST;
	CK_X_ASSERTION_TYPE atype = CKT_X_PINNED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE check = 0;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	CK_ATTRIBUTE lookup[] = {
		{ CKA_CERT_SHA1_HASH, (void*)SHA1_CHECKSUM, XL (SHA1_CHECKSUM) },
		{ CKA_CLASS, &nklass, sizeof (nklass) },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, lookup, G_N_ELEMENTS (lookup));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, &check, 1, &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (check, !=, 0);
	gkm_assert_cmpulong (n_objects, >, 0);
}

TESTING_TEST (trust_create_assertion_missing_type)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	/* Missing CKT_X_ANCHORED_CERTIFICATE */
	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_TEMPLATE_INCOMPLETE);
}

TESTING_TEST (trust_create_assertion_bad_type)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = 0xFFFF;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	/* Missing CKA_X_CERTIFICATE_VALUE */
	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_TEMPLATE_INCONSISTENT);
}

TESTING_TEST (trust_create_assertion_missing_cert_value)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_ANCHORED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	/* Missing CKA_X_CERTIFICATE_VALUE */
	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_TEMPLATE_INCOMPLETE);
}

TESTING_TEST (trust_create_assertion_bad_cert_value)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_ANCHORED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	/* Bad CKA_X_CERTIFICATE_VALUE */
	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, "12", 2 },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_ATTRIBUTE_VALUE_INVALID);
}

TESTING_TEST (trust_create_assertion_null_cert_value)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_ANCHORED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	/* Bad CKA_X_CERTIFICATE_VALUE */
	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, NULL, 0 },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_ATTRIBUTE_VALUE_INVALID);
}

TESTING_TEST (trust_create_assertion_for_distrusted)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_DISTRUSTED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE check = 0;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) }
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, &check, 1, &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_objects, ==, 1);
	gkm_assert_cmpulong (check, ==, object);
}

TESTING_TEST (trust_create_assertion_for_distrusted_no_purpose)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_DISTRUSTED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) }
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_TEMPLATE_INCOMPLETE);
}

TESTING_TEST (trust_create_assertion_for_distrusted_no_serial)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_DISTRUSTED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) }
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_TEMPLATE_INCOMPLETE);
}

TESTING_TEST (trust_create_assertion_twice)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_DISTRUSTED_CERTIFICATE;
	CK_OBJECT_HANDLE object_1 = 0;
	CK_OBJECT_HANDLE object_2 = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) }
	};

	/* First object should go away when we create an overlapping assertion */

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object_1);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object_1, !=, 0);

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object_2);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object_2, !=, 0);

	gkm_assert_cmpulong (object_1, !=, object_2);

	/* First object no longer exists */
	rv = gkm_session_C_DestroyObject (session, object_1);
	gkm_assert_cmprv (rv, ==, CKR_OBJECT_HANDLE_INVALID);
}

TESTING_TEST (trust_distrusted_assertion_has_no_cert_value)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_DISTRUSTED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_ATTRIBUTE check;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "test-purpose", 12 },
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) }
	};

	/* Created as distrusted, should have no CKA_X_CERTIFICATE_VALUE */

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	check.type = CKA_X_CERTIFICATE_VALUE;
	check.pValue = NULL;
	check.ulValueLen = 0;
	rv = gkm_session_C_GetAttributeValue (session, object, &check, 1);
	gkm_assert_cmprv (rv, ==, CKR_ATTRIBUTE_TYPE_INVALID);
}

TESTING_TEST (trust_create_assertion_complete_on_token)
{
	CK_OBJECT_CLASS klass = CKO_X_TRUST_ASSERTION;
	CK_X_ASSERTION_TYPE atype = CKT_X_PINNED_CERTIFICATE;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE check = 0;
	CK_OBJECT_HANDLE results[8];
	CK_BBOOL token = CK_TRUE;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &klass, sizeof (klass) },
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_X_PURPOSE, "other", 5 },
		{ CKA_TOKEN, &token, sizeof (token) },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &check);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (check, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, results, G_N_ELEMENTS (results), &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	/* Second should have overwritten the first */
	gkm_assert_cmpulong (n_objects, ==, 1);
	gkm_assert_cmpulong (results[0], ==, check);
}

TESTING_TEST (trust_destroy_assertion_on_token)
{
	CK_X_ASSERTION_TYPE atype = CKT_X_PINNED_CERTIFICATE;
	CK_OBJECT_HANDLE results[8];
	CK_BBOOL token = CK_TRUE;
	CK_ULONG n_objects = 0;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_ASSERTION_TYPE, &atype, sizeof (atype) },
		{ CKA_TOKEN, &token, sizeof (token) },
	};

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, results, G_N_ELEMENTS (results), &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_objects, ==, 1);

	rv = gkm_session_C_DestroyObject (session, results[0]);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	rv = gkm_session_C_FindObjectsInit (session, attrs, G_N_ELEMENTS (attrs));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, results, G_N_ELEMENTS (results), &n_objects);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_objects, ==, 0);
}

static void
_assert_positive_netscape (CK_X_ASSERTION_TYPE assertion_type, const gchar *purpose,
                           CK_ATTRIBUTE_TYPE netscape_type, CK_TRUST netscape_trust,
                           const gchar *description)
{
	CK_OBJECT_CLASS aklass = CKO_X_TRUST_ASSERTION;
	CK_OBJECT_CLASS nklass = CKO_NETSCAPE_TRUST;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE results[256];
	CK_ULONG n_results = 0;
	CK_BYTE checksum[20];
	gsize n_checksum;
	CK_ATTRIBUTE attr;
	CK_TRUST check;
	GChecksum *md;
	CK_BBOOL fval = CK_FALSE;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_X_CERTIFICATE_VALUE, cert_data, n_cert_data },
		{ CKA_CLASS, &aklass, sizeof (aklass) },
		{ CKA_X_ASSERTION_TYPE, &assertion_type, sizeof (assertion_type) },
		{ CKA_X_PURPOSE, (void*)purpose, strlen (purpose) },
		{ CKA_TOKEN, &fval, sizeof (fval) },
	};

	CK_ATTRIBUTE lookup[] = {
		{ CKA_CLASS, &nklass, sizeof (nklass) },
		{ CKA_TOKEN, &fval, sizeof (fval) },
		{ CKA_CERT_SHA1_HASH, checksum, sizeof (checksum) },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	md = g_checksum_new (G_CHECKSUM_SHA1);
	g_checksum_update (md, cert_data, n_cert_data);
	n_checksum = sizeof (checksum);
	g_checksum_get_digest (md, checksum, &n_checksum);
	g_assert (n_checksum == sizeof (checksum));

	rv = gkm_session_C_FindObjectsInit (session, lookup, G_N_ELEMENTS (lookup));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, results, G_N_ELEMENTS (results), &n_results);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_results, ==, 1);

	check = (CK_ULONG)-1;
	attr.type = netscape_type;
	attr.pValue = &check;
	attr.ulValueLen = sizeof (check);

	rv = gkm_session_C_GetAttributeValue (session, results[0], &attr, 1);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	if (check != netscape_trust)
		g_warning ("netscape trust was not mapped correctly: \"%s\"", description);
	gkm_assert_cmpulong (check, ==, netscape_trust);
}

static void
_assert_negative_netscape (CK_X_ASSERTION_TYPE assertion_type, const gchar *purpose,
                           CK_ATTRIBUTE_TYPE netscape_type, CK_TRUST netscape_trust,
                           const gchar *description)
{
	CK_OBJECT_CLASS aklass = CKO_X_TRUST_ASSERTION;
	CK_OBJECT_CLASS nklass = CKO_NETSCAPE_TRUST;
	CK_OBJECT_HANDLE object = 0;
	CK_OBJECT_HANDLE results[256];
	CK_ULONG n_results = 0;
	CK_ATTRIBUTE attr;
	CK_TRUST check;
	CK_BBOOL fval = CK_FALSE;
	CK_RV rv;

	CK_ATTRIBUTE attrs[] = {
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) },
		{ CKA_CLASS, &aklass, sizeof (aklass) },
		{ CKA_X_ASSERTION_TYPE, &assertion_type, sizeof (assertion_type) },
		{ CKA_X_PURPOSE, (void*)purpose, strlen (purpose) },
		{ CKA_TOKEN, &fval, sizeof (fval) },
	};

	CK_ATTRIBUTE lookup[] = {
		{ CKA_CLASS, &nklass, sizeof (nklass) },
		{ CKA_TOKEN, &fval, sizeof (fval) },
		{ CKA_SERIAL_NUMBER, (void*)SERIAL_NUMBER, XL (SERIAL_NUMBER) },
		{ CKA_ISSUER, (void*)DER_ISSUER, XL (DER_ISSUER) },
	};

	rv = gkm_session_C_CreateObject (session, attrs, G_N_ELEMENTS (attrs), &object);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	gkm_assert_cmpulong (object, !=, 0);

	rv = gkm_session_C_FindObjectsInit (session, lookup, G_N_ELEMENTS (lookup));
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjects (session, results, G_N_ELEMENTS (results), &n_results);
	gkm_assert_cmprv (rv, ==, CKR_OK);
	rv = gkm_session_C_FindObjectsFinal (session);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	gkm_assert_cmpulong (n_results, ==, 1);

	check = (CK_ULONG)-1;
	attr.type = netscape_type;
	attr.pValue = &check;
	attr.ulValueLen = sizeof (check);

	rv = gkm_session_C_GetAttributeValue (session, results[0], &attr, 1);
	gkm_assert_cmprv (rv, ==, CKR_OK);

	if (check != netscape_trust)
		g_warning ("netscape trust was not mapped correctly: \"%s\"", description);
	gkm_assert_cmpulong (check, ==, netscape_trust);
}

/* Some macros for intelligent failure messages */
#define assert_positive_netscape(a, b, c, d) \
	_assert_positive_netscape (a, b, c, d, #a ", " #b ", " #c ", " #d)
#define assert_negative_netscape(a, b, c, d) \
	_assert_negative_netscape (a, b, c, d, #a ", " #b ", " #c ", " #d)

TESTING_TEST (trust_netscape_map_server_auth)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.1",
	                          CKA_TRUST_SERVER_AUTH, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.1",
	                          CKA_TRUST_SERVER_AUTH, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.1",
	                          CKA_TRUST_SERVER_AUTH, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_client_auth)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.2",
	                          CKA_TRUST_CLIENT_AUTH, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.2",
	                          CKA_TRUST_CLIENT_AUTH, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.2",
	                          CKA_TRUST_CLIENT_AUTH, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_code_signing)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.3",
	                          CKA_TRUST_CODE_SIGNING, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.3",
	                          CKA_TRUST_CODE_SIGNING, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.3",
	                          CKA_TRUST_CODE_SIGNING, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_email)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.4",
	                          CKA_TRUST_EMAIL_PROTECTION, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.4",
	                          CKA_TRUST_EMAIL_PROTECTION, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.4",
	                          CKA_TRUST_EMAIL_PROTECTION, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_ipsec_endpoint)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.5",
	                          CKA_TRUST_IPSEC_END_SYSTEM, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.5",
	                          CKA_TRUST_IPSEC_END_SYSTEM, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.5",
	                          CKA_TRUST_IPSEC_END_SYSTEM, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_ipsec_tunnel)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.6",
	                          CKA_TRUST_IPSEC_TUNNEL, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.6",
	                          CKA_TRUST_IPSEC_TUNNEL, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.6",
	                          CKA_TRUST_IPSEC_TUNNEL, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_ipsec_user)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.7",
	                          CKA_TRUST_IPSEC_USER, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.7",
	                          CKA_TRUST_IPSEC_USER, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.7",
	                          CKA_TRUST_IPSEC_USER, CKT_NETSCAPE_UNTRUSTED);
}

TESTING_TEST (trust_netscape_map_time_stamping)
{
	assert_positive_netscape (CKT_X_PINNED_CERTIFICATE, "1.3.6.1.5.5.7.3.8",
	                          CKA_TRUST_TIME_STAMPING, CKT_NETSCAPE_TRUSTED);
	assert_positive_netscape (CKT_X_ANCHORED_CERTIFICATE, "1.3.6.1.5.5.7.3.8",
	                          CKA_TRUST_TIME_STAMPING, CKT_NETSCAPE_TRUSTED_DELEGATOR);
	assert_negative_netscape (CKT_X_DISTRUSTED_CERTIFICATE, "1.3.6.1.5.5.7.3.8",
	                          CKA_TRUST_TIME_STAMPING, CKT_NETSCAPE_UNTRUSTED);
}
