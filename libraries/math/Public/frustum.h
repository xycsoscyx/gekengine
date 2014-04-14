#pragma once

template <typename TYPE>
struct tfrustum
{
public:
    tvector3<TYPE> origin;
    tplane<TYPE> planes[6];

public:
    tfrustum(void)
    {
    }

    void Create(const tmatrix4x4<TYPE> &nTransform)
    {
        // Extract the origin;
        origin = nTransform.t;

        // Extract the left plane
	    planes[0].normal.x = (nTransform._14 + nTransform._11);
	    planes[0].normal.y = (nTransform._24 + nTransform._21);
	    planes[0].normal.z = (nTransform._34 + nTransform._31);
	    planes[0].distance = (nTransform._44 + nTransform._41);
	    planes[0].Normalize();

        // Extract the right plane
	    planes[1].normal.x = (nTransform._14 - nTransform._11);
	    planes[1].normal.y = (nTransform._24 - nTransform._21);
	    planes[1].normal.z = (nTransform._34 - nTransform._31);
	    planes[1].distance = (nTransform._44 - nTransform._41);
	    planes[1].Normalize();

        // Extract the bottom plane
	    planes[2].normal.x = (nTransform._14 + nTransform._12);
	    planes[2].normal.y = (nTransform._24 + nTransform._22);
	    planes[2].normal.z = (nTransform._34 + nTransform._32);
	    planes[2].distance = (nTransform._44 + nTransform._42);
	    planes[2].Normalize();

        // Extract the top plane
	    planes[3].normal.x = (nTransform._14 - nTransform._12);
	    planes[3].normal.y = (nTransform._24 - nTransform._22);
	    planes[3].normal.z = (nTransform._34 - nTransform._32);
	    planes[3].distance = (nTransform._44 - nTransform._42);
	    planes[3].Normalize();

        // Extract the far plane
	    planes[4].normal.x = (nTransform._14 - nTransform._13);
	    planes[4].normal.y = (nTransform._24 - nTransform._23);
	    planes[4].normal.z = (nTransform._34 - nTransform._33);
	    planes[4].distance = (nTransform._44 - nTransform._43);
	    planes[4].Normalize();

        // Extract the near plane
	    planes[5].normal.x = nTransform._13;
	    planes[5].normal.y = nTransform._23;
	    planes[5].normal.z = nTransform._33;
	    planes[5].distance = nTransform._43;
	    planes[5].Normalize();
    }

	bool IsVisible(const tvector3<TYPE> &nPoint) const
    {
        if (planes[0].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
        else if (planes[1].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
        else if (planes[2].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
        else if (planes[3].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
        else if (planes[4].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
        else if (planes[5].CheckCollision(nPoint) == GEK_OBJECT_BEHIND) return false;
	    return true;
    }

	bool IsVisible(const tsphere<TYPE> &nSphere) const
    {
        if (planes[0].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
        else if (planes[1].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
        else if (planes[2].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
        else if (planes[3].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
        else if (planes[4].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
        else if (planes[5].CheckCollision(nSphere) == GEK_OBJECT_BEHIND) return false;
	    return true;
    }

    bool IsVisible(const taabb<TYPE> &nBox) const
    {
        if (planes[0].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[1].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[2].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[3].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[4].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[5].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
	    return true;
    }

    bool IsVisible(const tobb<TYPE> &nBox) const
    {
        if (planes[0].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[1].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[2].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[3].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[4].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
        else if (planes[5].CheckCollision(nBox) == GEK_OBJECT_BEHIND) return false;
	    return true;
    }
};
