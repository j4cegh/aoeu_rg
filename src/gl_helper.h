#pragma once

#define GL_RECT2QUAD(rect, col) \
{ \
glColor4ub(color4_2func_alpha(col)); \
glBegin(GL_QUADS); \
    glVertex2f(rect.left, rect.top); \
    glVertex2f(rect.width, rect.top); \
    glVertex2f(rect.width, rect.height); \
    glVertex2f(rect.left, rect.height); \
glEnd(); \
}

#define GL_LINEBOX(rect, col) \
{ \
glColor4ub(color4_2func_alpha(col)); \
glBegin(GL_LINE_STRIP); \
    glVertex2f(rect.left, rect.top); \
    glVertex2f(rect.left + rect.width, rect.top); \
    glVertex2f(rect.left + rect.width, rect.top + rect.height); \
    glVertex2f(rect.left, rect.top + rect.height); \
    glVertex2f(rect.left, rect.top); \
glEnd(); \
}

#define GL_DRAWLINE(p1, p2, col) \
{ \
glColor4ub(color4_2func_alpha(col)); \
glBegin(GL_LINES); \
    glVertex2f(p1.x, p1.y); \
    glVertex2f(p2.x, p2.y); \
glEnd(); \
}

#define GL_RECT2BOX(rect, col) GL_RECT2QUAD(FLOATRECT(rect.left, rect.top, rect.left + rect.width, rect.top + rect.height), col)

#define GL_OLBOX(rect, ocol, mcol, opad) \
{ \
GL_RECT2BOX(FLOATRECT(rect.left - opad, rect.top - opad, rect.width + (opad * 2), rect.height + (opad * 2)), ocol) \
GL_RECT2BOX(FLOATRECT(rect.left, rect.top, rect.width, rect.height), mcol) \
}

#define GL_LINESTRIP(rect, col) \
{ \
glColor4ub(color4_2func_alpha(col)); \
glBegin(GL_LINE_STRIP); \
    glVertex2f(rect.left, rect.top); \
    glVertex2f(rect.width, rect.top); \
    glVertex2f(rect.width, rect.height); \
    glVertex2f(rect.left, rect.height); \
    glVertex2f(rect.left, rect.top); \
glEnd(); \
}

#define GL_CIRCLE(x, y, r, pc, a, col) \
{ \
glColor4ub(color4_2func_alpha(col)); \
glBegin(GL_TRIANGLE_FAN); \
    glVertex2f(x, y); \
    int i; \
    for(i = 0; i <= pc * a;i++) {  \
        glVertex2f( \
                x + (r * cos(i *  (2.0f * PI_NUM) / pc)), \
            y + (r * sin(i * (2.0f * PI_NUM) / pc)) \
        ); \
    } \
glEnd(); \
}

#define GL_SCISSOR(rect) \
glScissor( \
    (rect.left - rg.gui_view_xoff) / rg.gui_view_scale, \
    (rg.win_height / rg.gui_view_scale) - ((rect.top + rect.height - rg.gui_view_yoff) / rg.gui_view_scale), \
    rect.width / rg.gui_view_scale, \
    rect.height / rg.gui_view_scale \
);

#define GL_SCISSOR_COORDS(x, y, width, height) GL_SCISSOR(FLOATRECT(x, y, width, height))
#define GL_TESTSCISSOR \
{ \
    float_rect wrect = FLOATRECT(0, 0, rg.win_width, rg.win_height); \
	GL_RECT2QUAD(wrect, color4_mod_alpha(col_yellow, 150)) \
}

#define GL_SCALE(x, y, scale) \
{ \
    glPushMatrix(); \
    glTranslatef(x, y, 0); \
    glScalef(scale, scale, 0); \
}

#define GL_SCALE_END() glPopMatrix();

#define GL_ROT(x, y, rot) \
{ \
    glPushMatrix(); \
    glTranslatef(x, y, 0); \
    glRotatef(rot, 0.0, 0.0, 1.0); \
}

#define GL_ROT_END() glPopMatrix();
