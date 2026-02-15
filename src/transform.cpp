#include "hurdygurdy.hpp"

void HgTransform::create_child(HgEntity child) {
    HgTransform& old_first = hg_ecs->get<HgTransform>(first_child);
    HgTransform& new_first = hg_ecs->get<HgTransform>(child);
    old_first.prev_sibling = child;
    new_first.next_sibling = first_child;
    first_child = child;
}

void HgTransform::detach() {
    if (parent != HgEntity{}) {
        if (first_child == HgEntity{}) {
            if (prev_sibling == HgEntity{}) {
                hg_ecs->get<HgTransform>(parent).first_child = next_sibling;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = HgEntity{};
            } else {
                hg_ecs->get<HgTransform>(prev_sibling).next_sibling = next_sibling;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = prev_sibling;
            }
        } else {
            HgEntity last_child = first_child;
            for (;;) {
                HgTransform& tf = hg_ecs->get<HgTransform>(last_child);
                tf.parent = parent;
                if (tf.next_sibling == HgEntity{})
                    break;
                last_child = tf.next_sibling;
            }
            if (prev_sibling == HgEntity{}) {
                hg_ecs->get<HgTransform>(parent).first_child = first_child;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = last_child;
            } else {
                hg_ecs->get<HgTransform>(prev_sibling).next_sibling = first_child;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = last_child;
            }
        }
    } else {
        HgEntity child = first_child;
        while (child != HgEntity{}) {
            HgTransform& tf = hg_ecs->get<HgTransform>(child);
            child = tf.next_sibling;
            tf.parent = HgEntity{};
            tf.next_sibling = HgEntity{};
            tf.prev_sibling = HgEntity{};
        }
    }
}

void HgTransform::destroy() {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = hg_ecs->get<HgTransform>(child);
        tf.destroy();
        child = tf.next_sibling;
    }
    if (parent != HgEntity{}) {
        if (prev_sibling != HgEntity{}) {
            hg_ecs->get<HgTransform>(prev_sibling).next_sibling = next_sibling;
            if (next_sibling != HgEntity{})
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = prev_sibling;
        } else {
            hg_ecs->get<HgTransform>(parent).first_child = next_sibling;
            if (next_sibling != HgEntity{})
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = HgEntity{};
        }
    }
    hg_ecs->despawn(hg_ecs->get_entity(*this));
}

void HgTransform::set(const HgVec3& p, const HgVec3& s, const HgQuat& r) {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = hg_ecs->get<HgTransform>(child);
        // tf.set(
        //     hg_rotate(r, (tf.position - position) * s / tf.scale + p),
        //     s * tf.scale / scale,
        //     r);
        child = tf.next_sibling;
    }
    position = p;
    scale = s;
    rotation = r;
}

void HgTransform::move(const HgVec3& dp, const HgVec3& ds, const HgQuat& dr) {
    set(position + dp, scale * ds, dr * rotation);
}

