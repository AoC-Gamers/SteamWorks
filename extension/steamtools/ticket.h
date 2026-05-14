/*
    This file is part of SourcePawn SteamWorks.

    SourcePawn SteamWorks is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, as per version 3 of the License.

    SourcePawn SteamWorks is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourcePawn SteamWorks.  If not, see <http://www.gnu.org/licenses/>.
	
	Author: Kyle Sanderson (KyleS).
*/

#pragma once

#include "blob.h"
class GCTokenSection
{
public:
	GCTokenSection(CBlob &blob)
	{
		blob.Read(length);
		
		if (length != expectedlen)
		{
			bValid = false;
			return;
		}
		
		blob.Read(token);
		blob.Read(steamid);
		bValid = blob.Read(generation);
	};
public:
	bool IsValid() const
	{
		return bValid;
	}

public:  /* No ideal packing, but it saves us from having to comment. */
	static const uint32 expectedlen = 20; /* Magic number for GC? */
	uint32 length;
	uint64 token;
	CSteamID steamid;
	time_t generation;
	bool bValid;
};

class SessionSection
{
public:
	SessionSection(CBlob &blob)
	{
		blob.Read(length);
		
		if (length != expectedlen)
		{
			bValid = false;
			return;
		}
		
		blob.Read(unk1);
		blob.Read(unk2);
		blob.Read(externalip);
		blob.Read(filler);
		blob.Read(timestamp);
		bValid = blob.Read(connectioncount);
	}
public:
	bool IsValid() const
	{
		return bValid;
	}

public:
	static const uint32 expectedlen = 24; /* Magic number for Session? */
	uint32 length;
	uint32 unk1;
	uint32 unk2;
	uint32 externalip;
	uint32 filler;
	uint32 timestamp;
	uint32 connectioncount;
	bool bValid;
};

class DLCInfo
{
public:
	DLCInfo() : pSub(nullptr)
	{
	};
	
	~DLCInfo()
	{
		delete [] pSub;
	};
public:
	void TakeBlob(CBlob &blob)
	{
		blob.Read(appid);
		blob.Read(subcount);
		
		delete [] pSub;
		pSub = new uint32[subcount];

		if (pSub == nullptr)
		{
			return;
		}

		for (size_t iter = 0; iter < subcount; ++iter)
		{
			blob.Read(pSub[iter]);
		}
	}
public:
	uint32 appid;
	uint16 subcount;
	uint32 *pSub;
};

class OwnershipSection
{
public:
	OwnershipSection(CBlob &blob)
	{
		blob.Read(length);
		
		if (length == 0)
		{
			pLicense = nullptr;
			pDLC = nullptr;
			bValid = false;
			return;
		}
		
		blob.Read(length);
		blob.Read(version);
		blob.Read(steamid);
		blob.Read(appid);
		blob.Read(externalIP);
		blob.Read(internalIP);
		blob.Read(ownershipFlags);
		blob.Read(ticketGeneration);
		blob.Read(ticketExpiration);
		blob.Read(licenseCount);
		
		pLicense = new uint32[licenseCount];

		if (pLicense == nullptr)
		{
			pDLC = nullptr;
			bValid = false;
			return;
		}

		for (size_t iter = 0; iter < licenseCount; ++iter)
		{
			blob.Read(pLicense[iter]);
		}
		
		blob.Read(dlcCount);
		pDLC = new DLCInfo[dlcCount];
		if (pDLC == nullptr)
		{
			bValid = false;
			return;
		}
		
		for (size_t iter = 0; iter < dlcCount; ++iter)
		{
			pDLC[iter].TakeBlob(blob);
		}
		
		blob.Read(reserved);
		bValid = blob.Read(signature, sizeof(signature));
	};
	
	~OwnershipSection()
	{
		delete [] pLicense;
		delete [] pDLC;
	}
public:
	bool IsValid() const
	{
		return bValid;
	}

public:
	uint32 length;
	uint32 version;
	uint64 steamid;
	uint32 appid;
	uint32 externalIP;
	uint32 internalIP;
	uint32 ownershipFlags;
	time_t ticketGeneration;
	time_t ticketExpiration;
	uint16 licenseCount;
	uint32 *pLicense;
	uint16 dlcCount;
	DLCInfo *pDLC;
	uint16 reserved;
	unsigned char signature[128];
	bool bValid;
};

class AuthBlob /* Concept and CBlob adapted from SteamTools; Ticket Information from OpenSteamWorks. */
{
public:
	AuthBlob(const void *pAuthTicket, int cbAuthTicket)
	{
		CBlob blob(pAuthTicket, cbAuthTicket);
		pGCTokenSection = new GCTokenSection(blob);
		pSessionSection = new SessionSection(blob);
		pOwnershipSection = new OwnershipSection(blob);
		
		bExpectedTicket = (pGCTokenSection->IsValid() && pSessionSection->IsValid() && pOwnershipSection->IsValid());
	}
	
	~AuthBlob()
	{
		delete pGCTokenSection;
		delete pSessionSection;
		delete pOwnershipSection;
	}
public:
	GCTokenSection *pGCTokenSection = nullptr;
	SessionSection *pSessionSection = nullptr;
	OwnershipSection *pOwnershipSection = nullptr;
	bool bExpectedTicket = false;
};
